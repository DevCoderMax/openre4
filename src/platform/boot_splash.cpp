// Splash de boot do OpenRE4: mostra assets/boot.png em tela cheia (cover-fit)
// por cima do motor, com fade-out, até o tempo esgotar ou o jogador apertar algo.
//
// Temporario por design: vira o menu nativo (splash + botoes ImGui) na proxima etapa.
#include <aurora/imgui.h>
#include <imgui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OPENRE4_BOOT_RGBA
#define OPENRE4_BOOT_RGBA "build/boot_assets/boot.rgba"
#endif
#ifndef OPENRE4_BOOT_META
#define OPENRE4_BOOT_META "build/boot_assets/boot.meta"
#endif

namespace {

ImTextureID g_bootTex = (ImTextureID) 0;
int g_bootW = 0;
int g_bootH = 0;
bool g_bootTried = false;
float g_bootTime = 0.0f;

void boot_try_load()
{
    g_bootTried = true;
    FILE* meta = fopen(OPENRE4_BOOT_META, "r");
    if (meta != NULL) {
        if (fscanf(meta, "%d %d", &g_bootW, &g_bootH) != 2) {
            g_bootW = g_bootH = 0;
        }
        fclose(meta);
    }
    if (g_bootW <= 0 || g_bootH <= 0) {
        return;
    }
    FILE* f = fopen(OPENRE4_BOOT_RGBA, "rb");
    if (f == NULL) {
        g_bootW = g_bootH = 0;
        return;
    }
    size_t n = (size_t) g_bootW * (size_t) g_bootH * 4;
    void* rgba = malloc(n);
    if (rgba == NULL) {
        fclose(f);
        return;
    }
    if (fread(rgba, 1, n, f) != n) {
        free(rgba);
        fclose(f);
        g_bootW = g_bootH = 0;
        return;
    }
    fclose(f);
    g_bootTex = aurora_imgui_add_texture((uint32_t) g_bootW, (uint32_t) g_bootH, rgba);
    free(rgba);
    if (g_bootTex == (ImTextureID) 0) {
        g_bootW = g_bootH = 0;
    }
}

} // namespace

// Textura do boot para reuso (menu nativo). Carrega sob demanda; 0 se indisponivel.
ImTextureID openre4_boot_texture(int* w, int* h)
{
    if (!g_bootTried) {
        boot_try_load();
    }
    if (w) {
        *w = g_bootW;
    }
    if (h) {
        *h = g_bootH;
    }
    return g_bootTex;
}

// Desenha o splash. Retorna false quando acabou (some) e o jogo segue normal.
// dt: segundos desde o frame anterior. skip: true quando o jogador apertou algo.
bool openre4_boot_splash(float dt, bool skip)
{
    if (!g_bootTried) {
        boot_try_load();
    }
    if (g_bootTex == (ImTextureID) 0) {
        return false;
    }
    g_bootTime += dt;
    // Visivel por ~6s com fade-out no ultimo 1,5s; qualquer tecla/botao pula.
    const float kShowTime = 6.0f;
    const float kFadeTime = 1.5f;
    float alpha = 1.0f;
    if (g_bootTime > kShowTime - kFadeTime) {
        alpha = (kShowTime - g_bootTime) / kFadeTime;
    }
    if (skip || g_bootTime >= kShowTime || alpha <= 0.0f) {
        return false;
    }

    ImVec2 vp = ImGui::GetMainViewport()->Size;
    float imgAspect = (float) g_bootW / (float) g_bootH;
    float vpAspect = vp.x > 0.0f && vp.y > 0.0f ? vp.x / vp.y : imgAspect;
    ImVec2 size;
    ImVec2 uv0(0.0f, 0.0f), uv1(1.0f, 1.0f);
    if (vpAspect > imgAspect) {
        // Viewport mais largo: preenche largura, corta topo/base.
        float crop = 1.0f - imgAspect / vpAspect;
        uv0.y = crop * 0.5f;
        uv1.y = 1.0f - crop * 0.5f;
        size = vp;
    } else {
        // Viewport mais alto: preenche altura, corta laterais.
        float crop = 1.0f - vpAspect / imgAspect;
        uv0.x = crop * 0.5f;
        uv1.x = 1.0f - crop * 0.5f;
        size = vp;
    }

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(vp);
    ImGui::Begin("openre4_boot", NULL,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::Image(g_bootTex, size, uv0, uv1);
    if (alpha < 1.0f) {
        ImU32 fade = IM_COL32(0, 0, 0, (int) ((1.0f - alpha) * 255.0f));
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0.0f, 0.0f), vp, fade);
    }
    ImGui::End();
    return true;
}
