// Menu nativo do OpenRE4 integrado com a crate Rust (crates/openre4_ui).
//
// Fundo estilizado com a arte de boot + botões START, LOAD e OPTIONS renderizados
// pelo ImGui com animações e lógica de estado fornecidas 100% pelo Rust via FFI.
#include <aurora/imgui.h>
#include <imgui.h>

#include "global.h"
#include "joy.h"
#include "openre4_ui.h"

#include <stdio.h>

// Publicados por Title_task (title.cpp).
extern int g_openre4_titleRno0;
extern int g_openre4_titleRno1;
extern int g_openre4_titleCursor;
extern int g_openre4_titleMenuNum;

// Textura do fundo (boot_splash.cpp).
ImTextureID openre4_boot_texture(int* w, int* h);

// Mantido para compatibilidade com pad.cpp
unsigned openre4_menu_consume()
{
    return 0;
}

namespace {

bool menu_in_title()
{
    return (pG != NULL) && StaFlagChk(pG, STA_TITLE);
}

} // namespace

// Desenha a camada do menu nativo em Rust. Chamar todo frame entre bio4_frame() e aurora_end_frame().
void openre4_menu_layer()
{
    if (!menu_in_title()) {
        ImVec2 vp = ImGui::GetMainViewport()->Size;
        if (vp.x <= 0.0f || vp.y <= 0.0f) {
            vp = ImVec2(1280.0f, 960.0f);
        }

        u16 cur_room = (pG != NULL) ? pG->room_id : 0;
        const char* room_name = "Desconhecida";
        if (cur_room == 0x120) {
            room_name = "r120 (Estrada Montanhosa / Intro da Viatura)";
        } else if (cur_room == 0x100) {
            room_name = "r100 (Caminho da Floresta / 1ª Casa Ganado)";
        } else if (cur_room == 0x101) {
            room_name = "r101 (Vila Central / El Pueblo - Casas e Torre)";
        }

        if (openre4_camera_is_freecam_active()) {
            OpenRE4CameraDebugInfo dbg = {};
            openre4_camera_get_debug_info(&dbg);

            ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f));
            ImGui::SetNextWindowSize(ImVec2(vp.x - 32.0f, 0.0f));
            ImGui::Begin("freecam_hud", NULL,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing);

            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "[F3] MODO VOO LIVRE / FREECAM ATIVO (Rust Engine + glam)");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), " | Sala: %s", room_name);
            ImGui::Separator();
            ImGui::Text("Pos: (%.1f, %.1f, %.1f) | Alvo: (%.1f, %.1f, %.1f) | FOV: %.1f deg",
                        dbg.pos_x, dbg.pos_y, dbg.pos_z, dbg.target_x, dbg.target_y, dbg.target_z, dbg.fovy);
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.9f),
                               "Voo: WASD (Mover) | Mouse/Setas (Olhar) | Espaco/Ctrl (Subir/Descer) | Shift (Boost 3x) | Scroll (Zoom) | [F3] Sair");
            ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f),
                               "Teletransporte de Sala: [F4] Estrada r120 | [F5] 1ª Casa r100 | [F6] Vila Central r101 (Casas/Torre)");
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f),
                               "Presets de Camera: [1] Praca Central | [2] Vista Aerea | [3] Estrada | [4] 1ª Casa");
            ImGui::End();
        } else if (cur_room == 0x120) {
            // Dica de navegação quando em r120 (cutscene não-jogável)
            ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f));
            ImGui::SetNextWindowSize(ImVec2(vp.x - 32.0f, 0.0f));
            ImGui::Begin("r120_info_hud", NULL,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[CENA INTRO] Sala atual: %s", room_name);
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 0.9f),
                               "Camera do Jogo: Fixa na cutscene da viatura (Leon ainda nao jogavel nesta cena)");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f),
                               "[Espaco / Enter / Esc]: Pular Cutscene e entrar na Vila (r100)");
            ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f),
                               "[F3]: Ativar Modo Voo Livre | [F5]: Teleportar 1ª Casa | [F6]: Teleportar Vila Central (Casas)");
            ImGui::End();
        }
        return;
    }

    int imgW = 0, imgH = 0;
    ImTextureID bg = openre4_boot_texture(&imgW, &imgH);
    if (bg == (ImTextureID) 0 || imgW <= 0 || imgH <= 0) {
        return;
    }

    ImVec2 vp = ImGui::GetMainViewport()->Size;
    if (vp.x <= 0.0f || vp.y <= 0.0f) {
        vp = ImVec2(1280.0f, 960.0f);
    }
    float imgAspect = (float) imgW / (float) imgH;
    float vpAspect = vp.x / vp.y;
    ImVec2 uv0(0.0f, 0.0f), uv1(1.0f, 1.0f);
    if (vpAspect > imgAspect) {
        float crop = 1.0f - imgAspect / vpAspect;
        uv0.y = crop * 0.5f;
        uv1.y = 1.0f - crop * 0.5f;
    } else {
        float crop = 1.0f - vpAspect / imgAspect;
        uv0.x = crop * 0.5f;
        uv1.x = 1.0f - crop * 0.5f;
    }

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(vp);
    ImGui::Begin("openre4_menu", NULL,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::Image(bg, vp, uv0, uv1);

    // Os botões interativos só aparecem quando a tarefa de título está no menu pronto
    bool show_buttons = (g_openre4_titleRno0 == 5) && openre4_ui_is_active();
    if (show_buttons) {
        const char* labels[3] = {"START", "LOAD", "OPTIONS"};
        ImVec2 btnSize(vp.x * 0.30f, vp.y * 0.075f);
        float x = (vp.x - btnSize.x) * 0.5f;
        float y = vp.y * 0.62f;
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::BeginGroup();

        // Atualiza estado do Rust
        float dt = ImGui::GetIO().DeltaTime;
        bool key_up = ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_W);
        bool key_down = ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_S);
        bool key_select = ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space);

        openre4_ui_update(dt, key_up ? 1 : 0, key_down ? 1 : 0, key_select ? 1 : 0);

        for (int i = 0; i < 3; i++) {
            if (i > 0) {
                ImGui::Spacing();
            }

            float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
            openre4_ui_get_button_color(i, &r, &g, &b, &a);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(r, g, b, a));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.15f, g * 1.15f, b * 1.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

            if (ImGui::Button(labels[i], btnSize)) {
                openre4_ui_on_click(i);
            }

            if (ImGui::IsItemHovered()) {
                openre4_ui_on_hover(i, 1);
            } else {
                openre4_ui_on_hover(i, 0);
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
        }
        ImGui::EndGroup();

        // Rodapé sutil com identificação do motor
        ImGui::SetCursorPos(ImVec2(16.0f, vp.y - 28.0f));
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.6f), "OpenRE4 Native Engine (PC x86_64) | UI Core in Rust");
    }

    ImGui::End();
}
