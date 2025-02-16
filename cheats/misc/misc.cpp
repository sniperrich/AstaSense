﻿// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "misc.h"
#include "fakelag.h"
#include "..\ragebot\aim.h"
#include "..\visuals\world_esp.h"
#include "prediction_system.h"
#include "logs.h"
#include "..\visuals\hitchams.h"
#include "../menu_alpha.h"
#include "../Configuration/Config.h"

#define ALPHA (ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar| ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)
#define NOALPHA (ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)

void misc::EnableHiddenCVars()
{
	auto p = **reinterpret_cast<ConCommandBase***>(reinterpret_cast<DWORD>(m_cvar()) + 0x34);

	for (auto c = p->m_pNext; c != nullptr; c = c->m_pNext) {
		c->m_nFlags &= ~FCVAR_DEVELOPMENTONLY; // FCVAR_DEVELOPMENTONLY
		c->m_nFlags &= ~FCVAR_HIDDEN; // FCVAR_HIDDEN
	}
}

void misc::pingspike()
{
	int value = c_config::get()->i["misc_pingspike_val"] / 2;
	ConVar* net_fakelag = m_cvar()->FindVar(crypt_str("net_fakelag"));
	if (c_config::get()->b["misc_ping_spike"] && c_config::get()->auto_check(c_config::get()->i["ping_spike_key"], c_config::get()->i["ping_spike_key_style"]))
		net_fakelag->SetValue(value);
	else
	{
		net_fakelag->SetValue(0);
	}
}

void misc::NoDuck(CUserCmd* cmd)
{
	if (!c_config::get()->b["inf_duck"])
		return;

	if (m_gamerules()->m_bIsValveDS())
		return;

	cmd->m_buttons |= IN_BULLRUSH;
}

void misc::AutoCrouch(CUserCmd* cmd)
{
	if (fakelag::get().condition)
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (m_gamerules()->m_bIsValveDS())
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!c_config::get()->auto_check(c_config::get()->i["rage_fd_enabled"], c_config::get()->i["rage_fd_enabled_style"]))
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!g_ctx.globals.fakeducking && m_clientstate()->iChokedCommands != 7)
		return;

	if (m_clientstate()->iChokedCommands >= 7)
		cmd->m_buttons |= IN_DUCK;
	else
		cmd->m_buttons &= ~IN_DUCK;

	g_ctx.globals.fakeducking = true;
}

void misc::SlideWalk(CUserCmd* cmd)
{
	if (!g_ctx.local()->is_alive()) //-V807
		return;

	if (g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (antiaim::get().condition(cmd, true) && (g_cfg.misc.legs_movement == 2 && math::random_int(0, 2) == 0 || g_cfg.misc.legs_movement == 1))
	{
		if (cmd->m_forwardmove > 0.0f)
		{
			cmd->m_buttons |= IN_BACK;
			cmd->m_buttons &= ~IN_FORWARD;
		}
		else if (cmd->m_forwardmove < 0.0f)
		{
			cmd->m_buttons |= IN_FORWARD;
			cmd->m_buttons &= ~IN_BACK;
		}

		if (cmd->m_sidemove > 0.0f)
		{
			cmd->m_buttons |= IN_MOVELEFT;
			cmd->m_buttons &= ~IN_MOVERIGHT;
		}
		else if (cmd->m_sidemove < 0.0f)
		{
			cmd->m_buttons |= IN_MOVERIGHT;
			cmd->m_buttons &= ~IN_MOVELEFT;
		}
	}
	else
	{
		auto buttons = cmd->m_buttons & ~(IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD);

		if (c_config::get()->i["leg_movementtype"] == 1 && math::random_int(0, 1)) //|| c_config::get()->i["leg_movementtype"] == 1)
		{
			if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
				return;

			if (cmd->m_forwardmove <= 0.0f)
				buttons |= IN_BACK;
			else
				buttons |= IN_FORWARD;

			if (cmd->m_sidemove > 0.0f)
				goto LABEL_15;
			else if (cmd->m_sidemove >= 0.0f)
				goto LABEL_18;

			goto LABEL_17;
		}
		else
			goto LABEL_18;

		if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
			return;

		if (cmd->m_forwardmove <= 0.0f) //-V779
			buttons |= IN_FORWARD;
		else
			buttons |= IN_BACK;

		if (cmd->m_sidemove > 0.0f)
		{
		LABEL_17:
			buttons |= IN_MOVELEFT;
			goto LABEL_18;
		}

		if (cmd->m_sidemove < 0.0f)
			LABEL_15:

		buttons |= IN_MOVERIGHT;

	LABEL_18:
		cmd->m_buttons = buttons;
	}
}

void misc::automatic_peek(CUserCmd* cmd, float wish_yaw)
{
	if (!g_ctx.globals.weapon->is_non_aim() && c_config::get()->b["rage_quick_peek_assist"] && c_config::get()->auto_check(c_config::get()->i["rage_quickpeek_enabled"], c_config::get()->i["rage_quickpeek_enabled_style"]))
	{
		if (g_ctx.globals.start_position.IsZero())
		{
			g_ctx.globals.start_position = g_ctx.local()->GetAbsOrigin();

			if (!(engineprediction::get().backup_data.flags & FL_ONGROUND))
			{
				Ray_t ray;
				CTraceFilterWorldAndPropsOnly filter;
				CGameTrace trace;

				ray.Init(g_ctx.globals.start_position, g_ctx.globals.start_position - Vector(0.0f, 0.0f, 1000.0f));
				m_trace()->TraceRay(ray, MASK_SOLID, &filter, &trace);

				if (trace.fraction < 1.0f)
					g_ctx.globals.start_position = trace.endpos + Vector(0.0f, 0.0f, 2.0f);
			}
		}
		else
		{
			auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (cmd->m_buttons & IN_ATTACK || cmd->m_buttons & IN_ATTACK2);

			if (cmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
				g_ctx.globals.fired_shot = true;

			if (g_ctx.globals.fired_shot)
			{
				auto &current_position = g_ctx.local()->GetAbsOrigin();
				auto difference = current_position - g_ctx.globals.start_position;

				if (difference.Length2D() > 5.0f)
				{
					auto velocity = Vector(difference.x * cos(wish_yaw / 180.0f * M_PI) + difference.y * sin(wish_yaw / 180.0f * M_PI), difference.y * cos(wish_yaw / 180.0f * M_PI) - difference.x * sin(wish_yaw / 180.0f * M_PI), difference.z);

					cmd->m_forwardmove = -velocity.x * 50.0f;
					cmd->m_sidemove = velocity.y * 50.0f;
				}
				else
				{
					g_ctx.globals.fired_shot = false;
					g_ctx.globals.start_position.Zero();
				}
			}
		}
	}
	else
	{
		g_ctx.globals.fired_shot = false;
		g_ctx.globals.start_position.Zero();
	}
}

void misc::ViewModel()
{
	if (g_cfg.esp.viewmodel_fov)
	{
		auto viewFOV = (float)g_cfg.esp.viewmodel_fov + 68.0f;
		static auto viewFOVcvar = m_cvar()->FindVar(crypt_str("viewmodel_fov"));

		if (viewFOVcvar->GetFloat() != viewFOV) //-V550
		{
			*(float*)((DWORD)&viewFOVcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewFOVcvar->SetValue(viewFOV);
		}
	}

	if (g_cfg.esp.viewmodel_x)
	{
		auto viewX = (float)g_cfg.esp.viewmodel_x / 2.0f;
		static auto viewXcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_x")); //-V807

		if (viewXcvar->GetFloat() != viewX) //-V550
		{
			*(float*)((DWORD)&viewXcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewXcvar->SetValue(viewX);
		}
	}

	if (g_cfg.esp.viewmodel_y)
	{
		auto viewY = (float)g_cfg.esp.viewmodel_y / 2.0f;
		static auto viewYcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_y"));

		if (viewYcvar->GetFloat() != viewY) //-V550
		{
			*(float*)((DWORD)&viewYcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewYcvar->SetValue(viewY);
		}
	}

	if (g_cfg.esp.viewmodel_z)
	{
		auto viewZ = (float)g_cfg.esp.viewmodel_z / 2.0f;
		static auto viewZcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_z"));

		if (viewZcvar->GetFloat() != viewZ) //-V550
		{
			*(float*)((DWORD)&viewZcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewZcvar->SetValue(viewZ);
		}
	}
}

void misc::FullBright()
{
	if (!c_config::get()->auto_check(c_config::get()->i["esp_en"], c_config::get()->i["esp_en_type"]))
		return;

	static auto mat_fullbright = m_cvar()->FindVar(crypt_str("mat_fullbright"));

	if (mat_fullbright->GetBool() != c_config::get()->m["brightadj"][0])
		mat_fullbright->SetValue(c_config::get()->m["brightadj"][0]);
}

void misc::DrawGray()
{
	if (!c_config::get()->auto_check(c_config::get()->i["esp_en"], c_config::get()->i["esp_en_type"]))
		return;

	static auto mat_drawgray = m_cvar()->FindVar(crypt_str("mat_drawgray"));

	if (mat_drawgray->GetBool() != g_cfg.esp.drawgray)
		mat_drawgray->SetValue(g_cfg.esp.drawgray);
}

void misc::PovArrows(player_t* e, Color color)
{
	auto cfg = c_config::get();
	auto isOnScreen = [](Vector origin, Vector& screen) -> bool
	{
		if (!math::world_to_screen(origin, screen))
			return false;

		static int iScreenWidth, iScreenHeight;
		m_engine()->GetScreenSize(iScreenWidth, iScreenHeight);

		auto xOk = iScreenWidth > screen.x;
		auto yOk = iScreenHeight > screen.y;

		return xOk && yOk;
	};

	Vector screenPos;

	if (isOnScreen(e->GetAbsOrigin(), screenPos))
		return;

	Vector viewAngles;
	m_engine()->GetViewAngles(viewAngles);

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto screenCenter = Vector2D(width * 0.5f, height * 0.5f);
	auto angleYawRad = DEG2RAD(viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()).y - 90.0f);

	auto radius = cfg->i["oofradius"];
	auto size = cfg->i["oofsize"];

	auto newPointX = screenCenter.x + ((((width - (size * 4)) * 0.5f) * (radius / 200.0f)/*(radius / 100.0f)*/) * cos(angleYawRad)) + (int)(6.0f * (((float)size - 4.0f) / 16.0f));
	auto newPointY = screenCenter.y + ((((height - (size * 4)) * 0.5f) * (radius / 200.0f)/*(radius / 100.0f)*/) * sin(angleYawRad));

	std::array <Vector2D, 3> points
	{
		Vector2D(newPointX - size, newPointY - size),
		Vector2D(newPointX + size, newPointY),
		Vector2D(newPointX - size, newPointY + size)
	};

	math::rotate_triangle(points, viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()).y - 90.0f);
	render::get().triangle(points.at(0), points.at(1), points.at(2), color);
}

void misc::NightmodeFix()
{
	static auto in_game = false;

	if (m_engine()->IsInGame() && !in_game)
	{
		in_game = true;

		g_ctx.globals.change_materials = true;
		worldesp::get().changed = true;

		static auto skybox = m_cvar()->FindVar(crypt_str("sv_skyname"));
		worldesp::get().backup_skybox = skybox->GetString();
		return;
	}
	else if (!m_engine()->IsInGame() && in_game)
		in_game = false;

	static auto player_enable = c_config::get()->auto_check(c_config::get()->i["esp_en"], c_config::get()->i["esp_en_type"]);

	if (player_enable != c_config::get()->auto_check(c_config::get()->i["esp_en"], c_config::get()->i["esp_en_type"]))
	{
		player_enable = c_config::get()->auto_check(c_config::get()->i["esp_en"], c_config::get()->i["esp_en_type"]);
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting = c_config::get()->m["brightadj"][1];

	if (setting != c_config::get()->m["brightadj"][1])
	{
		setting = c_config::get()->m["brightadj"][1];
		g_ctx.globals.change_materials = true;
		return;
	}

	Color wrldcol
	{
		c_config::get()->c["adjcol"][0],
		c_config::get()->c["adjcol"][1],
		c_config::get()->c["adjcol"][2],
		c_config::get()->c["adjcol"][3]
	};

	static auto setting_world = wrldcol;

	if (setting_world != wrldcol)
	{
		setting_world = wrldcol;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting_props = g_cfg.esp.props_color;

	if (setting_props != g_cfg.esp.props_color)
	{
		setting_props = g_cfg.esp.props_color;
		g_ctx.globals.change_materials = true;
	}
}

void misc::aimbot_hitboxes()
{
	if (!c_config::get()->auto_check(c_config::get()->i["esp_en"], c_config::get()->i["esp_en_type"]))
		return;

	if (!g_cfg.player.lag_hitbox)
		return;

	auto player = (player_t*)m_entitylist()->GetClientEntity(aim::get().last_target_index);

	if (!player)
		return;

	auto model = player->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;

	auto hitbox_set = studio_model->pHitboxSet(player->m_nHitboxSet());

	if (!hitbox_set)
		return;

	hit_chams::get().add_matrix(player, aim::get().last_target[aim::get().last_target_index].record.matrixes_data.main);
}

void misc::rank_reveal()
{
	if (!c_config::get()->b["misc_rev_comp"])
		return;

	using RankReveal_t = bool(__cdecl*)(int*);
	static auto Fn = (RankReveal_t)(util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 A1 ? ? ? ? 85 C0 75 37")));

	int array[3] =
	{
		0,
		0,
		0
	};

	Fn(array);
}




void misc::fast_stop(CUserCmd* m_pcmd)
{
	if (!c_config::get()->b["rage_quickstop"])
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	auto pressed_move_key = m_pcmd->m_buttons & IN_FORWARD || m_pcmd->m_buttons & IN_MOVELEFT || m_pcmd->m_buttons & IN_BACK || m_pcmd->m_buttons & IN_MOVERIGHT || m_pcmd->m_buttons & IN_JUMP;

	if (pressed_move_key)
		return;

	if (!(g_cfg.antiaim.type[antiaim::get().type].desync && !g_cfg.antiaim.lby_type && (!g_ctx.globals.weapon->is_grenade() || g_cfg.esp.on_click && ~(m_pcmd->m_buttons & IN_ATTACK) && !(m_pcmd->m_buttons & IN_ATTACK2))) || antiaim::get().condition(m_pcmd)) //-V648
	{
		auto &velocity = g_ctx.local()->m_vecVelocity();

		if (velocity.Length2D() > 20.0f)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			m_pcmd->m_forwardmove = negative_forward_direction.x;
			m_pcmd->m_sidemove = negative_side_direction.y;


		}
	}
	else
	{
		auto &velocity = g_ctx.local()->m_vecVelocity();

		if (velocity.Length2D() > 20.0f)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			m_pcmd->m_forwardmove = negative_forward_direction.x;
			m_pcmd->m_sidemove = negative_side_direction.y;
		}
		else
		{
			auto speed = 1.01f;

			if (m_pcmd->m_buttons & IN_DUCK || g_ctx.globals.fakeducking)
				speed *= 2.94117647f;

			static auto switch_move = false;

			if (switch_move)
				m_pcmd->m_sidemove += speed;
			else
				m_pcmd->m_sidemove -= speed;

			switch_move = !switch_move;
		}
	}
}

void misc::spectators_list()
{
	//LPDIRECT3DTEXTURE9 photo[32];
	int specs = 0;
	int id[32];
	int modes = 0;
	std::string spect = "";
	std::string mode = "";

	if (m_engine()->IsInGame() && m_engine()->IsConnected())
	{
		int localIndex = m_engine()->GetLocalPlayer();
		player_t* pLocalEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(localIndex));
		if (pLocalEntity)
		{
			for (int i = 0; i < m_engine()->GetMaxClients(); i++)
			{
				player_t* pBaseEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(i));
				if (!pBaseEntity)
					continue;
				if (pBaseEntity->m_iHealth() > 0)
					continue;
				if (pBaseEntity == pLocalEntity)
					continue;
				if (pBaseEntity->IsDormant())
					continue;
				if (pBaseEntity->m_hObserverTarget() != pLocalEntity)
					continue;

				player_info_t pInfo;
				m_engine()->GetPlayerInfo(pBaseEntity->EntIndex(), &pInfo);
				if (pInfo.ishltv)
					continue;
				spect += pInfo.szName;
				spect += "\n";
				specs++;
				id[i] = pInfo.steamID64;
				// photo[i] = c_menu::get().steam_image(CSteamID((uint64)pInfo.steamID64));
				switch (pBaseEntity->m_iObserverMode())
				{
				case OBS_MODE_IN_EYE:
					mode += "Perspective";
					break;
				case OBS_MODE_CHASE:
					mode += "3rd Person";
					break;
				case OBS_MODE_ROAMING:
					mode += "No Clip";
					break;
				case OBS_MODE_DEATHCAM:
					mode += "Deathcam";
					break;
				case OBS_MODE_FREEZECAM:
					mode += "Freezecam";
					break;
				case OBS_MODE_FIXED:
					mode += "Fixed";
					break;
				default:
					break;
				}

				mode += "\n";
				modes++;



			}
		}
	}

	static float main_alpha = 0.f;
	if (!c_config::get()->b["spectators"])
		return;

	if (hooks::menu_open || specs > 0) {
		if (main_alpha < 1)
			main_alpha += 5 * ImGui::GetIO().DeltaTime;
	}
	else
		if (main_alpha > 0)
			main_alpha -= 5 * ImGui::GetIO().DeltaTime;

	if (main_alpha < 0.05f)
		return;

	ImVec2 p, s;

	ImGui::Begin("SPECTATORS", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNav);
	{
		auto d = ImGui::GetWindowDrawList();
		int think_size = 200;
		int calced_size = think_size - 5;

		p = ImGui::GetWindowPos();
		s = ImGui::GetWindowSize();
		ImGui::SetWindowSize(ImVec2(200, 31 + 20 * specs));
		ImGui::PushFont(c_menu::get().font);

		//d->AddRectFilled(p, p + ImVec2(200, 21 + 20 * specs), ImColor(39, 39, 39, int(50 * main_alpha)));
		auto main_colf = ImColor(39, 39, 39, int(255 * main_alpha));
		auto main_coll = ImColor(39, 39, 39, int(140 * main_alpha));
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf, main_colf, main_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf, main_coll, main_coll, main_colf);

		auto main_colf2 = ImColor(39, 39, 39, int(140 * min(main_alpha * 2, 1.f)));
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf2, main_colf2, main_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf2, main_coll, main_coll, main_colf2);
		auto line_colf = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), int(200 * min(main_alpha * 2, 1.f)));
		auto line_coll = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), int(255 * min(main_alpha * 2, 1.f)));
		d->AddRectFilledMultiColor(p, p + ImVec2(100, 2), line_coll, line_colf, line_colf, line_coll);
		d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 2), line_colf, line_coll, line_coll, line_colf);
		d->AddText(p + ImVec2((200) / 2 - ImGui::CalcTextSize("spectators").x / 2, (20) / 2 - ImGui::CalcTextSize("spectators").y / 2), ImColor(250, 250, 250, int(230 * min(main_alpha * 3, 1.f))), "spectators");
		if (specs > 0) spect += "\n";
		if (modes > 0) mode += "\n";

		ImGui::SetCursorPosY(22);
		if (m_engine()->IsInGame() && m_engine()->IsConnected())
		{
			int localIndex = m_engine()->GetLocalPlayer();
			player_t* pLocalEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(localIndex));
			if (pLocalEntity)
			{
				for (int i = 0; i < m_engine()->GetMaxClients(); i++)
				{
					player_t* pBaseEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(i));
					if (!pBaseEntity)
						continue;
					if (pBaseEntity == pLocalEntity)
						continue;
					if (pBaseEntity->m_hObserverTarget() != pLocalEntity)
						continue;

					player_info_t pInfo;
					m_engine()->GetPlayerInfo(pBaseEntity->EntIndex(), &pInfo);
					if (pInfo.ishltv)
						continue;
					ImGui::SetCursorPosX(8);
					ImGui::Text(pInfo.szName);
					if (pInfo.fakeplayer)
					{
						//ImGui::SameLine(-1, s.x - 22);
						//ImGui::Image(getAvatarTexture(pBaseEntity->m_iTeamNum()), ImVec2(15, 15), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.f, 1.f, 1.f, min(main_alpha * 2, 1.f)));
					}
					else
					{
						//ImGui::SameLine(-1, s.x - 22);
						//ImGui::Image(c_menu::get().steam_image(CSteamID((uint64)pInfo.steamID64)), ImVec2(15, 15), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.f, 1.f, 1.f, min(main_alpha * 2, 1.f)));
					}

				}
			}
		}

		ImGui::PopFont();
	}
	ImGui::End();
}

enum key_bind_num
{
	_AUTOFIRE,
	_LEGITBOT,
	_DOUBLETAP,
	_SAFEPOINT,
	_MIN_DAMAGE,
	_ANTI_BACKSHOT = 12,
	_M_BACK,
	_M_LEFT,
	_M_RIGHT,
	_DESYNC_FLIP,
	_THIRDPERSON,
	_AUTO_PEEK,
	_EDGE_JUMP,
	_FAKEDUCK,
	_SLOWWALK,
	_BODY_AIM,
	_RAGEBOT,
	_TRIGGERBOT,
	_L_RESOLVER_OVERRIDE,
	_FAKE_PEEK,
};

void misc::double_tap_deffensive(CUserCmd* m_pcmd)
{
	if (c_config::get()->i["rage_dtmode"] != 0 || !g_ctx.send_packet)
		return;

	// check weapon -- revolver off
	const auto pCombatWeapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!pCombatWeapon || pCombatWeapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER || (m_pcmd->m_buttons & IN_ATTACK))
		return;

	g_ctx.globals.tickbase_shift = 13;

	if (g_ctx.globals.m_Peek.m_bIsPeeking) // проверка на пик, это тока обявление -- сам код из аимбота надо взять из hentaiware.
	{
		g_ctx.globals.tickbase_shift++;
		g_ctx.globals.tickbase_shift = min(g_ctx.globals.tickbase_shift, 14); // сам шифт дефенсива, потом ласт шифт на 14 тиков.
	}
	else
		g_ctx.globals.tickbase_shift = 0;
}

bool misc::double_tap(CUserCmd* m_pcmd)
{
	
	//if (c_config::get()->b["rage_dt"] && c_config::get()->auto_check(c_config::get()->i["rage_dt_key"], c_config::get()->i["rage_dt_key_style"])) {
	if (!g_ctx.globals.weapon->is_non_aim() && (c_config::get()->b["rage_dt"] && c_config::get()->auto_check(c_config::get()->i["rage_dt_key"], c_config::get()->i["rage_dt_key_style"]))) 
	{
		double_tap_enabled = true;

		static auto recharge_double_tap = false;
		static auto last_double_tap = 0;

		if (recharge_double_tap)
		{
			recharging_double_tap = true;
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.new_dt.charge_ticks = 0;
			g_ctx.globals.next_tickbase_shift = 0;
			recharge_double_tap = false;
			return false;
		}

		if (recharging_double_tap)
		{
			auto recharge_time = g_ctx.globals.weapon->can_double_tap() ? TIME_TO_TICKS(0.5f) : TIME_TO_TICKS(1.f);

			if (!aim::get().should_stop && fabs(g_ctx.globals.fixed_tickbase - last_double_tap) > recharge_time)
			{
				recharging_double_tap = false;
				double_tap_key = true;
				hide_shots_key = false;
			}
			else if (m_pcmd->m_buttons & IN_ATTACK)
				last_double_tap = g_ctx.globals.fixed_tickbase;
		}

		if (!c_config::get()->b["rage_enabled"])
		{
			double_tap_enabled = false;
			double_tap_key = false;
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.new_dt.charge_ticks = 0;
			g_ctx.globals.next_tickbase_shift = 0;
			return false;
		}

		if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN) //-V807
		{
			double_tap_enabled = false;
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.new_dt.charge_ticks = 0;
			g_ctx.globals.next_tickbase_shift = 0;
			return false;
		}

		if (m_gamerules()->m_bIsValveDS())
		{
			double_tap_enabled = false;
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.new_dt.charge_ticks = 0;
			g_ctx.globals.next_tickbase_shift = 6;
			return false;
		}

		if (g_ctx.globals.fakeducking)
		{
			double_tap_enabled = false;
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.new_dt.charge_ticks = 0;
			g_ctx.globals.next_tickbase_shift = 0;
			return false;
		}

		auto max_tickbase_shift = g_ctx.globals.weapon->get_max_tickbase_shift();

		if (antiaim::get().freeze_check)
			return true;

		if (!g_ctx.globals.weapon->is_grenade() && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_C4 && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_HEALTHSHOT && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_TASER && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && !g_ctx.globals.weapon->is_knife() && g_ctx.send_packet && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife())) //-V648
		{
			auto next_command_number = m_pcmd->m_command_number + 1;
			auto user_cmd = m_input()->GetUserCmd(next_command_number);

			memcpy(user_cmd, m_pcmd, sizeof(CUserCmd)); //-V598
			user_cmd->m_command_number = next_command_number;

			util::copy_command(user_cmd, max_tickbase_shift);

			if (g_ctx.globals.aimbot_working)
			{
				g_ctx.globals.double_tap_aim = true;
				g_ctx.globals.double_tap_aim_check = true;
			}

			recharge_double_tap = true;
			double_tap_enabled = false;
			double_tap_key = false;
			last_double_tap = g_ctx.globals.fixed_tickbase;
		}
		else if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_C4 && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_HEALTHSHOT && !g_ctx.globals.weapon->is_grenade()&& g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
			g_ctx.globals.tickbase_shift = max_tickbase_shift;

		return true;
	}

}


void misc::hide_shots(CUserCmd* m_pcmd, bool should_work)
{
	hide_shots_enabled = true;

	if ((!c_config::get()->b["rage_enabled"] && !c_config::get()->auto_check(c_config::get()->i["rage_key_enabled"], c_config::get()->i["rage_key_enabled_st"])))
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.next_tickbase_shift = 0;
		}

		return;
	}
	
	if (!c_config::get()->i["hs_key"])
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.next_tickbase_shift = 0;
		}

		return;
	}

	if (!c_config::get()->auto_check(c_config::get()->i["hs_key"],c_config::get()->i["hs_key_style"]))
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.next_tickbase_shift = 0;
		}

		return;
	}

	if (!should_work && double_tap_key)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;
		return;
	}

	if (!hide_shots_key)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.next_tickbase_shift = 0;
		return;
	}

	double_tap_key = false;

	if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.next_tickbase_shift = 0;
		return;
	}

	if (g_ctx.globals.fakeducking)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.next_tickbase_shift = 0;
		return;
	}

	if (antiaim::get().freeze_check)
		return;

	g_ctx.globals.next_tickbase_shift = m_gamerules()->m_bIsValveDS() ? 6 : 9;

	auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);
	auto weapon_shoot = m_pcmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife() || revolver_shoot;

	if (g_ctx.send_packet && !g_ctx.globals.weapon->is_grenade() && weapon_shoot)
		g_ctx.globals.tickbase_shift = g_ctx.globals.next_tickbase_shift;
}






	