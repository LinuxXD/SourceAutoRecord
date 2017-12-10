#pragma once
#include "Modules/Client.hpp"
#include "Modules/ConCommandArgs.hpp"
#include "Modules/Console.hpp"
#include "Modules/DemoRecorder.hpp"
#include "Modules/Engine.hpp"
#include "Modules/InputSystem.hpp"

#include "Demo.hpp"
#include "Rebinder.hpp"
#include "Summary.hpp"
#include "Timer.hpp"
#include "TimerAverage.hpp"
#include "TimerCheckPoints.hpp"
#include "Utils.hpp"

namespace Callbacks
{
	void PrintSession()
	{
		Console::Msg("Session Tick: %i (%.3f)\n", Engine::GetTick(), Engine::GetTime());
		//Console::Msg("Recorder Tick: %i\n", DemoRecorder::GetCurrentTick());
	}
	void PrintAbout()
	{
		Console::Msg("SourceAutoRecord tells the engine to keep recording when loading a save.\n");
		Console::Msg("More information at: https://nekzor.github.io/SourceAutoRecord\n");
		Console::Msg("Variant: %s\n", Offsets::GetStringVariant());
		Console::Msg("Version: %s\n", SAR_VERSION);
		Console::Msg("Build: %s\n", SAR_BUILD);
	}
	// Demo parsing
	namespace
	{
		void PrintDemoInfo(const void* ptr)
		{
			ConCommandArgs args(ptr);
			if (args.Count() != 2) {
				Console::Msg("sar_time_demo [demo_name] : Parses a demo and prints some information about it.\n");
				return;
			}

			std::string name;
			if (args.At(1)[0] == '\0') {
				if (DemoRecorder::DemoName[0] != '\0' && !DemoRecorder::LastDemo.empty()) {
					name = DemoRecorder::LastDemo;
				}
				else if (DemoPlayer::DemoName[0] != '\0') {
					name = std::string(DemoPlayer::DemoName);
				}
				else {
					Console::Msg("No demo was recorded or played back!\n");
					return;
				}
			}
			else {
				name = std::string(args.At(1));
			}

			Demo demo;
			if (demo.Parse(Engine::GetDir() + "\\" + name, sar_time_demo_dev.GetInt())) {
				demo.Fix();
				Console::Msg("Demo: %s\n", name.c_str());
				Console::Msg("Client: %s\n", demo.clientName);
				Console::Msg("Map: %s\n", demo.mapName);
				Console::Msg("Ticks: %i\n", demo.playbackTicks);
				Console::Msg("Time: %.3f\n", demo.playbackTime);
				Console::Msg("IpT: %.6f\n", demo.IntervalPerTick());
			}
			else {
				Console::Msg("Could not parse \"%s\"!\n", name.c_str());
			}
		}
		void PrintDemoInfos(const void* ptr)
		{
			ConCommandArgs args(ptr);

			int totalTicks = 0;
			float totalTime = 0;
			bool printTotal = false;

			std::string name;
			std::string dir = Engine::GetDir() + std::string("\\");
			for (size_t i = 1; i < args.Count(); i++)
			{
				name = std::string(args.At(i));

				Demo demo;
				if (demo.Parse(dir + name)) {
					demo.Fix();
					Console::Msg("Demo: %s\n", name.c_str());
					Console::Msg("Client: %s\n", demo.clientName);
					Console::Msg("Map: %s\n", demo.mapName);
					Console::Msg("Ticks: %i\n", demo.playbackTicks);
					Console::Msg("Time: %.3f\n", demo.playbackTime);
					Console::Msg("IpT: %.6f\n", demo.IntervalPerTick());
					Console::Msg("---------------\n");
					totalTicks += demo.playbackTicks;
					totalTime += demo.playbackTime;
					printTotal = true;
				}
				else {
					Console::Msg("Could not parse \"%s\"!\n", name.c_str());
				}
			}
			if (printTotal) {
				Console::Msg("Total Ticks: %i\n", totalTicks);
				Console::Msg("Total Time: %.3f\n", totalTime);
			}
		}
	}
	// Rebinder
	namespace
	{
		void BindSaveRebinder(const void* ptr)
		{
			ConCommandArgs args(ptr);
			if (args.Count() != 3) {
				Console::Msg("sar_bind_save <key> [save_name] : Automatic save rebinding when server has loaded. File indexing will be synced when recording demos.\n");
				return;
			}

			int button = InputSystem::StringToButtonCode(args.At(1));
			if (button == BUTTON_CODE_INVALID) {
				Console::Msg("\"%s\" isn't a valid key!\n", args.At(1));
				return;
			}
			else if (button == KEY_ESCAPE) {
				Console::Msg("Can't bind ESCAPE key!\n", args.At(1));
				return;
			}

			if (Rebinder::IsReloadBinding && button == Rebinder::ReloadButton) {
				Rebinder::ResetReloadBind();
			}

			Rebinder::SetSaveBind(button, args.At(2));
			Rebinder::RebindSave();
		}
		void BindReloadRebinder(const void* ptr)
		{
			ConCommandArgs args(ptr);
			if (args.Count() != 3) {
				Console::Msg("sar_bind_reload <key> [save_name] : Automatic save-reload rebinding when server has loaded. File indexing will be synced when recording demos.\n");
				return;
			}

			int button = InputSystem::StringToButtonCode(args.At(1));
			if (button == BUTTON_CODE_INVALID) {
				Console::Msg("\"%s\" isn't a valid key!\n", args.At(1));
				return;
			}
			else if (button == KEY_ESCAPE) {
				Console::Msg("Can't bind ESCAPE key!\n", args.At(1));
				return;
			}

			if (Rebinder::IsSaveBinding && button == Rebinder::SaveButton) {
				Rebinder::ResetSaveBind();
			}

			Rebinder::SetReloadBind(button, args.At(2));
			Rebinder::RebindReload();
		}
		void UnbindSaveRebinder()
		{
			if (!Rebinder::IsSaveBinding) {
				Console::Msg("There's nothing to unbind.\n");
				return;
			}
			Rebinder::ResetSaveBind();
		}
		void UnbindReloadRebinder()
		{
			if (!Rebinder::IsReloadBinding) {
				Console::Msg("There's nothing to unbind.\n");
				return;
			}
			Rebinder::ResetReloadBind();
		}
	}
	// Summary
	namespace
	{
		void StartSummary()
		{
			if (Summary::IsRunning) {
				Console::Msg("Summary has already started!\n");
				return;
			}
			Summary::Start();
		}
		void StopSummary()
		{
			if (!Summary::IsRunning) {
				Console::Msg("There's no summary to stop!\n");
				return;
			}

			if (sar_sum_during_session.GetBool()) {
				Summary::Add(Engine::GetTick(), Engine::GetTime(), *Engine::Mapname);
			}
			Summary::IsRunning = false;
		}
		void PrintSummary()
		{
			int sessions = Summary::Items.size();
			if (Summary::IsRunning && sessions == 0) {
				Console::Msg("Summary of this session:\n");
			}
			else if (Summary::IsRunning && sessions > 0) {
				Console::Msg("Summary of %i sessions:\n", sessions + 1);
			}
			else if (sessions > 0) {
				Console::Msg("Summary of %i session%s:\n", sessions, (sessions == 1) ? "" : "s");
			}
			else {
				Console::Msg("There's no result of a summary!\n");
				return;
			}

			for (size_t i = 0; i < Summary::Items.size(); i++) {
				Console::Msg("%s -> ", Summary::Items[i].Map);
				Console::Msg("%i ticks", Summary::Items[i].Ticks);
				Console::Msg("(%.3f)\n", Summary::Items[i].Time);
			}

			if (Summary::IsRunning) {
				Console::ColorMsg(COL_YELLOW, "%s -> ", *Engine::Mapname);
				Console::ColorMsg(COL_YELLOW, "%i ticks ", Engine::GetTick());
				Console::ColorMsg(COL_YELLOW, "(%.3f)\n", Engine::GetTime());
				Console::Msg("---------------\n");
				Console::Msg("Total Ticks: %i ", Summary::TotalTicks);
				Console::ColorMsg(COL_YELLOW, "(%i)\n", Summary::TotalTicks + Engine::GetTick());
				Console::Msg("Total Time: %.3f ", Summary::TotalTime);
				Console::ColorMsg(COL_YELLOW, "(%.3f)\n", Summary::TotalTime + Engine::GetTime());
			}
			else {
				Console::Msg("---------------\n");
				Console::Msg("Total Ticks: %i\n", Summary::TotalTicks);
				Console::Msg("Total Time: %.3f\n", Summary::TotalTime);
			}
		}
	}
	// Timer
	namespace
	{
		void StartTimer()
		{
			if (Timer::IsRunning)
				Console::DevMsg("Restarting timer!\n");
			else
				Console::DevMsg("Starting timer!\n");
			Timer::Start(Engine::GetTick());
		}
		void StopTimer()
		{
			if (!Timer::IsRunning) {
				Console::DevMsg("Timer isn't running!\n");
				return;
			}

			Timer::Stop(Engine::GetTick());

			if (Timer::Average::IsEnabled) {
				int tick = Timer::GetTick();
				Timer::Average::Add(tick, tick * *Engine::IntervalPerTick, *Engine::Mapname);
			}
		}
		void PrintTimer() {
			int tick = Timer::GetTick((Timer::IsRunning) ? Engine::GetTick() : -1);
			float time = tick * *Engine::IntervalPerTick;

			if (Timer::IsRunning) {
				Console::ColorMsg(COL_YELLOW, "Result: %i (%.3f)\n", tick, time);
			}
			else {
				Console::Msg("Result: %i (%.3f)\n", tick, time);
			}
		}
		void StartAverage() {
			Timer::Average::Start();
		}
		void StopAverage() {
			Timer::Average::IsEnabled = false;
		}
		void PrintAverage() {
			int average = Timer::Average::Items.size();
			if (average > 0) {
				Console::Msg("Average of %i:\n", average);
			}
			else {
				Console::Msg("No result!\n");
				return;
			}

			for (size_t i = 0; i < average; i++) {
				Console::Msg("%s -> ", Timer::Average::Items[i].Map);
				Console::Msg("%i ticks", Timer::Average::Items[i].Ticks);
				Console::Msg("(%.3f)\n", Timer::Average::Items[i].Time);
			}

			if (Timer::IsRunning) {
				Console::ColorMsg(COL_YELLOW, "Result: %i (%.3f)\n", Timer::Average::AverageTicks, Timer::Average::AverageTime);
			}
			else {
				Console::Msg("Result: %i (%.3f)\n", Timer::Average::AverageTicks, Timer::Average::AverageTime);
			}
		}
		void AddCheckpoint() {
			if (!Timer::IsRunning) {
				Console::DevMsg("Timer isn't running!\n");
				return;
			}

			int tick = Timer::GetTick(Engine::GetTick());
			Timer::CheckPoints::Add(tick, tick * *Engine::IntervalPerTick, *Engine::Mapname);
		}
		void ClearCheckpoints() {
			Timer::CheckPoints::Reset();
		}
		void PrintCheckpoints() {
			int cps = Timer::CheckPoints::Items.size();
			if (cps > 0) {
				Console::Msg("Result of %i checkpoint%s:\n", cps, (cps == 1) ? "" : "s");
			}
			else {
				Console::Msg("No result!\n");
				return;
			}

			for (size_t i = 0; i < cps; i++) {
				if (i == cps - 1 && Timer::IsRunning) {
					Console::ColorMsg(COL_YELLOW, "%s -> ", Timer::CheckPoints::Items[i].Map);
					Console::ColorMsg(COL_YELLOW, "%i ticks", Timer::CheckPoints::Items[i].Ticks);
					Console::ColorMsg(COL_YELLOW, "(%.3f)\n", Timer::CheckPoints::Items[i].Time);
				}
				else {
					Console::Msg("%s -> ", Timer::CheckPoints::Items[i].Map);
					Console::Msg("%i ticks", Timer::CheckPoints::Items[i].Ticks);
					Console::Msg("(%.3f)\n", Timer::CheckPoints::Items[i].Time);
				}
			}

			if (!Timer::IsRunning) {
				int tick = Timer::GetTick();
				float time = tick * *Engine::IntervalPerTick;
				Console::Msg("Result: %i (%.3f)\n", tick, time);
			}
		}
	}
}