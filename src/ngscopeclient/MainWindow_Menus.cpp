/***********************************************************************************************************************
*                                                                                                                      *
* glscopeclient                                                                                                        *
*                                                                                                                      *
* Copyright (c) 2012-2022 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Implementation of MainWindow
 */
#include "ngscopeclient.h"
#include "MainWindow.h"

#include "RemoteBridgeOscilloscope.h"

//Dock builder API is not yet public, so might change...
#include "imgui_internal.h"

//Dialogs
#include "AddGeneratorDialog.h"
#include "AddMultimeterDialog.h"
#include "AddPowerSupplyDialog.h"
#include "AddRFGeneratorDialog.h"
#include "AddScopeDialog.h"
#include "FilterGraphEditor.h"
#include "FunctionGeneratorDialog.h"
#include "HistoryDialog.h"
#include "LogViewerDialog.h"
#include "MetricsDialog.h"
#include "MultimeterDialog.h"
#include "PersistenceSettingsDialog.h"
#include "PreferenceDialog.h"
#include "ProtocolAnalyzerDialog.h"
#include "RFGeneratorDialog.h"
#include "SCPIConsoleDialog.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Top level menu

void MainWindow::AddDialog(shared_ptr<Dialog> dlg)
{
	m_dialogs.emplace(dlg);

	auto mdlg = dynamic_cast<MultimeterDialog*>(dlg.get());
	if(mdlg != nullptr)
		m_meterDialogs[mdlg->GetMeter()] = dlg;

	auto fdlg = dynamic_cast<FunctionGeneratorDialog*>(dlg.get());
	if(fdlg != nullptr)
		m_generatorDialogs[fdlg->GetGenerator()] = dlg;

	auto rdlg = dynamic_cast<RFGeneratorDialog*>(dlg.get());
	if(rdlg != nullptr)
		m_rfgeneratorDialogs[rdlg->GetGenerator()] = dlg;
}

/**
	@brief Run the top level menu bar
 */
void MainWindow::MainMenu()
{
	if(ImGui::BeginMainMenuBar())
	{
		FileMenu();
		ViewMenu();
		AddMenu();
		SetupMenu();
		WindowMenu();
		DebugMenu();
		HelpMenu();
		ImGui::EndMainMenuBar();
	}
}

/**
	@brief Run the File menu
 */
void MainWindow::FileMenu()
{
	if(ImGui::BeginMenu("File"))
	{
		if(ImGui::MenuItem("Close"))
			QueueCloseSession();

		ImGui::Separator();

		if(ImGui::MenuItem("Exit"))
			glfwSetWindowShouldClose(m_window, 1);

		ImGui::EndMenu();
	}
}

/**
	@brief Run the View menu
 */
void MainWindow::ViewMenu()
{
	if(ImGui::BeginMenu("View"))
	{
		if(ImGui::MenuItem("Fullscreen"))
			SetFullscreen(!m_fullscreen);

		ImGui::Separator();

		if(ImGui::MenuItem("Persistence Setup"))
		{
			m_persistenceDialog = make_shared<PersistenceSettingsDialog>(*this);
			AddDialog(m_persistenceDialog);
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add menu
 */
void MainWindow::AddMenu()
{
	if(ImGui::BeginMenu("Add"))
	{
		//Make a reverse mapping: timestamp -> instruments last used at that time
		map<time_t, vector<string> > reverseMap;
		for(auto it : m_recentInstruments)
			reverseMap[it.second].push_back(it.first);

		//Get a sorted list of timestamps, most recent first, with no duplicates
		set<time_t> timestampsDeduplicated;
		for(auto it : m_recentInstruments)
			timestampsDeduplicated.emplace(it.second);
		vector<time_t> timestamps;
		for(auto t : timestampsDeduplicated)
			timestamps.push_back(t);
		std::sort(timestamps.begin(), timestamps.end());

		AddGeneratorMenu(timestamps, reverseMap);
		AddMultimeterMenu(timestamps, reverseMap);
		AddOscilloscopeMenu(timestamps, reverseMap);
		AddPowerSupplyMenu(timestamps, reverseMap);
		AddRFGeneratorMenu(timestamps, reverseMap);

		ImGui::Separator();

		AddChannelsMenu();
		AddGenerateMenu();
		AddImportMenu();

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add | Generator menu
 */
void MainWindow::AddGeneratorMenu(vector<time_t>& timestamps, map<time_t, vector<string> >& reverseMap)
{
	if(ImGui::BeginMenu("Generator"))
	{
		if(ImGui::MenuItem("Connect..."))
			m_dialogs.emplace(make_shared<AddGeneratorDialog>(m_session));
		ImGui::Separator();

		//Find all known function generator drivers.
		//Any recent instrument using one of these drivers is assumed to be a generator.
		vector<string> drivers;
		SCPIFunctionGenerator::EnumDrivers(drivers);
		set<string> driverset;
		for(auto s : drivers)
			driverset.emplace(s);

		//Recent instruments
		for(int i=timestamps.size()-1; i>=0; i--)
		{
			auto t = timestamps[i];
			auto cstrings = reverseMap[t];
			for(auto cstring : cstrings)
			{
				auto fields = explode(cstring, ':');
				if(fields.size() < 4)
					continue;

				auto nick = fields[0];
				auto drivername = fields[1];
				auto transname = fields[2];

				if(driverset.find(drivername) != driverset.end())
				{
					if(ImGui::MenuItem(nick.c_str()))
					{
						auto path = fields[3];
						for(size_t j=4; j<fields.size(); j++)
							path = path + ":" + fields[j];

						auto transport = MakeTransport(transname, path);
						if(transport != nullptr)
						{
							//Create the scope
							auto gen = SCPIFunctionGenerator::CreateFunctionGenerator(drivername, transport);
							if(gen == nullptr)
							{
								ShowErrorPopup(
									"Driver error",
									"Failed to create function generator driver of type \"" + drivername + "\"");
								delete transport;
							}

							else
							{
								//TODO: apply preferences
								LogDebug("FIXME: apply PreferenceManager settings to newly created generator\n");

								gen->m_nickname = nick;
								m_session.AddFunctionGenerator(gen);
							}
						}
					}
				}
			}
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add | Multimeter menu
 */
void MainWindow::AddMultimeterMenu(vector<time_t>& timestamps, map<time_t, vector<string> >& reverseMap)
{
	if(ImGui::BeginMenu("Multimeter"))
	{
		if(ImGui::MenuItem("Connect..."))
			m_dialogs.emplace(make_shared<AddMultimeterDialog>(m_session));
		ImGui::Separator();

		//Find all known multimeter drivers.
		//Any recent instrument using one of these drivers is assumed to be a multimeter.
		vector<string> drivers;
		SCPIMultimeter::EnumDrivers(drivers);
		set<string> driverset;
		for(auto s : drivers)
			driverset.emplace(s);

		//Recent instruments
		for(int i=timestamps.size()-1; i>=0; i--)
		{
			auto t = timestamps[i];
			auto cstrings = reverseMap[t];
			for(auto cstring : cstrings)
			{
				auto fields = explode(cstring, ':');
				if(fields.size() < 4)
					continue;

				auto nick = fields[0];
				auto drivername = fields[1];
				auto transname = fields[2];

				if(driverset.find(drivername) != driverset.end())
				{
					if(ImGui::MenuItem(nick.c_str()))
					{
						auto path = fields[3];
						for(size_t j=4; j<fields.size(); j++)
							path = path + ":" + fields[j];

						auto transport = MakeTransport(transname, path);
						if(transport != nullptr)
						{
							//Create the scope
							auto meter = SCPIMultimeter::CreateMultimeter(drivername, transport);
							if(meter == nullptr)
							{
								ShowErrorPopup(
									"Driver error",
									"Failed to create multimeter driver of type \"" + drivername + "\"");
								delete transport;
							}

							else
							{
								//TODO: apply preferences
								LogDebug("FIXME: apply PreferenceManager settings to newly created meter\n");

								meter->m_nickname = nick;
								m_session.AddMultimeter(meter);
							}
						}
					}
				}
			}
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add | Oscilloscope menu
 */
void MainWindow::AddOscilloscopeMenu(vector<time_t>& timestamps, map<time_t, vector<string> >& reverseMap)
{
	if(ImGui::BeginMenu("Oscilloscope"))
	{
		if(ImGui::MenuItem("Connect..."))
			m_dialogs.emplace(make_shared<AddScopeDialog>(m_session));
		ImGui::Separator();

		//Find all known scope drivers.
		//Any recent instrument using one of these drivers is assumed to be a scope.
		vector<string> drivers;
		Oscilloscope::EnumDrivers(drivers);
		set<string> driverset;
		for(auto s : drivers)
			driverset.emplace(s);

		//Recent instruments
		for(int i=timestamps.size()-1; i>=0; i--)
		{
			auto t = timestamps[i];
			auto cstrings = reverseMap[t];
			for(auto cstring : cstrings)
			{
				auto fields = explode(cstring, ':');
				if(fields.size() < 3)
					continue;

				auto nick = fields[0];
				auto drivername = fields[1];
				auto transname = fields[2];

				if(driverset.find(drivername) != driverset.end())
				{
					if(ImGui::MenuItem(nick.c_str()))
					{
						string path;
						if(fields.size() >= 4)
							path = fields[3];
						for(size_t j=4; j<fields.size(); j++)
							path = path + ":" + fields[j];

						auto transport = MakeTransport(transname, path);
						if(transport != nullptr)
						{
							//Create the scope
							auto scope = Oscilloscope::CreateOscilloscope(drivername, transport);
							if(scope == nullptr)
							{
								ShowErrorPopup(
									"Driver error",
									"Failed to create oscilloscope driver of type \"" + drivername + "\"");
								delete transport;
							}

							else
							{
								m_session.ApplyPreferences(scope);
								scope->m_nickname = nick;
								m_session.AddOscilloscope(scope);
							}
						}
					}
				}
			}
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add | Power Supply menu
 */
void MainWindow::AddPowerSupplyMenu(vector<time_t>& timestamps, map<time_t, vector<string> >& reverseMap)
{
	if(ImGui::BeginMenu("Power Supply"))
	{
		if(ImGui::MenuItem("Connect..."))
			m_dialogs.emplace(make_shared<AddPowerSupplyDialog>(m_session));

		ImGui::Separator();

		//Find all known PSU drivers.
		//Any recent instrument using one of these drivers is assumed to be a PSU.
		vector<string> drivers;
		SCPIPowerSupply::EnumDrivers(drivers);
		set<string> driverset;
		for(auto s : drivers)
			driverset.emplace(s);

		//Recent instruments
		for(int i=timestamps.size()-1; i>=0; i--)
		{
			auto t = timestamps[i];
			auto cstrings = reverseMap[t];
			for(auto cstring : cstrings)
			{
				auto fields = explode(cstring, ':');
				if(fields.size() < 4)
					continue;

				auto nick = fields[0];
				auto drivername = fields[1];
				auto transname = fields[2];

				if(driverset.find(drivername) != driverset.end())
				{
					if(ImGui::MenuItem(nick.c_str()))
					{
						auto path = fields[3];
						for(size_t j=4; j<fields.size(); j++)
							path = path + ":" + fields[j];

						auto transport = MakeTransport(transname, path);
						if(transport != nullptr)
						{
							//Create the PSU
							auto psu = SCPIPowerSupply::CreatePowerSupply(drivername, transport);
							if(psu == nullptr)
							{
								ShowErrorPopup(
									"Driver error",
									"Failed to create PSU driver of type \"" + drivername + "\"");
								delete transport;
							}

							else
							{
								//TODO: apply preferences
								LogDebug("FIXME: apply PreferenceManager settings to newly created PSU\n");

								psu->m_nickname = nick;
								m_session.AddPowerSupply(psu);
							}
						}
					}
				}
			}
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add | RF Generator menu
 */
void MainWindow::AddRFGeneratorMenu(vector<time_t>& timestamps, map<time_t, vector<string> >& reverseMap)
{
	if(ImGui::BeginMenu("RF Generator"))
	{
		if(ImGui::MenuItem("Connect..."))
			m_dialogs.emplace(make_shared<AddRFGeneratorDialog>(m_session));
		ImGui::Separator();

		//Find all known function generator drivers.
		//Any recent instrument using one of these drivers is assumed to be a generator.
		vector<string> drivers;
		SCPIRFSignalGenerator::EnumDrivers(drivers);
		set<string> driverset;
		for(auto s : drivers)
			driverset.emplace(s);

		//Recent instruments
		for(int i=timestamps.size()-1; i>=0; i--)
		{
			auto t = timestamps[i];
			auto cstrings = reverseMap[t];
			for(auto cstring : cstrings)
			{
				auto fields = explode(cstring, ':');
				if(fields.size() < 4)
					continue;

				auto nick = fields[0];
				auto drivername = fields[1];
				auto transname = fields[2];

				if(driverset.find(drivername) != driverset.end())
				{
					if(ImGui::MenuItem(nick.c_str()))
					{
						auto path = fields[3];
						for(size_t j=4; j<fields.size(); j++)
							path = path + ":" + fields[j];

						auto transport = MakeTransport(transname, path);
						if(transport != nullptr)
						{
							//Create the scope
							auto gen = SCPIRFSignalGenerator::CreateRFSignalGenerator(drivername, transport);
							if(gen == nullptr)
							{
								ShowErrorPopup(
									"Driver error",
									"Failed to create RF generator driver of type \"" + drivername + "\"");
								delete transport;
							}

							else
							{
								//TODO: apply preferences
								LogDebug("FIXME: apply PreferenceManager settings to newly created RF generator\n");

								gen->m_nickname = nick;
								m_session.AddRFGenerator(gen);
							}
						}
					}
				}
			}
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add | Channels menu
 */
void MainWindow::AddChannelsMenu()
{
	if(ImGui::BeginMenu("Channels"))
	{
		for(auto scope : m_session.GetScopes())
		{
			if(ImGui::BeginMenu(scope->m_nickname.c_str()))
			{
				for(size_t i=0; i<scope->GetChannelCount(); i++)
				{
					auto chan = scope->GetChannel(i);
					for(size_t j=0; j<chan->GetStreamCount(); j++)
					{
						//skip trigger channels, those can't be displayed
						if(chan->GetType(j) == Stream::STREAM_TYPE_TRIGGER)
							continue;

						//Skip channels we can't enable
						if(!scope->CanEnableChannel(i))
							continue;

						StreamDescriptor stream(chan, j);
						if(ImGui::MenuItem(stream.GetName().c_str()))
						{
							auto group = GetBestGroupForWaveform(stream);
							auto area = make_shared<WaveformArea>(stream, group, this);
							group->AddArea(area);
						}
					}
				}

				ImGui::EndMenu();
			}
		}

		auto filters = Filter::GetAllInstances();
		for(auto f : filters)
		{
			for(size_t j=0; j<f->GetStreamCount(); j++)
			{
				StreamDescriptor stream(f, j);
				if(ImGui::MenuItem(stream.GetName().c_str()))
				{
					auto group = GetBestGroupForWaveform(stream);
					auto area = make_shared<WaveformArea>(stream, group, this);
					group->AddArea(area);
				}
			}
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add | Import menu
 */
void MainWindow::AddImportMenu()
{
	auto& refs = m_session.GetReferenceFilters();

	if(ImGui::BeginMenu("Import"))
	{
		//Find all filters in this category and sort them alphabetically
		vector<string> sortedNames;
		for(auto it : refs)
		{
			if(it.second->GetCategory() == Filter::CAT_GENERATION)
				sortedNames.push_back(it.first);
		}
		std::sort(sortedNames.begin(), sortedNames.end());

		//Do all of the menu items
		for(auto fname : sortedNames)
		{
			//Hide everything but import filters
			if(fname.find("Import") == string::npos)
				continue;

			string shortname = fname.substr(0, fname.size() - strlen(" Import"));

			if(ImGui::MenuItem(shortname.c_str()))
				CreateFilter(fname, nullptr, StreamDescriptor(nullptr, 0));
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Add | Generate menu
 */
void MainWindow::AddGenerateMenu()
{
	auto& refs = m_session.GetReferenceFilters();

	if(ImGui::BeginMenu("Generate"))
	{
		//Find all filters in this category and sort them alphabetically
		vector<string> sortedNames;
		for(auto it : refs)
		{
			if(it.second->GetCategory() == Filter::CAT_GENERATION)
				sortedNames.push_back(it.first);
		}
		std::sort(sortedNames.begin(), sortedNames.end());

		//Do all of the menu items
		for(auto fname : sortedNames)
		{
			//Hide import filters
			if(fname.find("Import") != string::npos)
				continue;

			//Hide filters that have inputs
			if(refs.find(fname)->second->GetInputCount() != 0)
				continue;

			if(ImGui::MenuItem(fname.c_str()))
				CreateFilter(fname, nullptr, StreamDescriptor(nullptr, 0));
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Setup menu
 */
void MainWindow::SetupMenu()
{
	if(ImGui::BeginMenu("Setup"))
	{
		bool timebaseVisible = (m_timebaseDialog != nullptr);
		if(timebaseVisible)
			ImGui::BeginDisabled();
		if(ImGui::MenuItem("Timebase..."))
			ShowTimebaseProperties();
		if(timebaseVisible)
			ImGui::EndDisabled();

		bool prefsVisible = (m_preferenceDialog != nullptr);
		if(prefsVisible)
			ImGui::BeginDisabled();
		if(ImGui::MenuItem("Preferences..."))
		{
			m_preferenceDialog = make_shared<PreferenceDialog>(m_session.GetPreferences());
			AddDialog(m_preferenceDialog);
		}
		if(prefsVisible)
			ImGui::EndDisabled();

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Window menu
 */
void MainWindow::WindowMenu()
{
	if(ImGui::BeginMenu("Window"))
	{
		WindowAnalyzerMenu();
		WindowGeneratorMenu();
		WindowMultimeterMenu();
		WindowSCPIConsoleMenu();

		bool hasLogViewer = m_logViewerDialog != nullptr;
		if(hasLogViewer)
			ImGui::BeginDisabled();

		if(ImGui::MenuItem("Log Viewer"))
		{
			m_logViewerDialog = make_shared<LogViewerDialog>(this);
			AddDialog(m_logViewerDialog);
		}

		if(hasLogViewer)
			ImGui::EndDisabled();

		bool hasMetrics = m_metricsDialog != nullptr;
		if(hasMetrics)
			ImGui::BeginDisabled();

		if(ImGui::MenuItem("Performance Metrics"))
		{
			m_metricsDialog = make_shared<MetricsDialog>(&m_session);
			AddDialog(m_metricsDialog);
		}

		if(hasMetrics)
			ImGui::EndDisabled();

		bool hasHistory = m_historyDialog != nullptr;
		if(hasHistory)
			ImGui::BeginDisabled();
		if(ImGui::MenuItem("History"))
		{
			m_historyDialog = make_shared<HistoryDialog>(m_session.GetHistory(), m_session, *this);
			AddDialog(m_historyDialog);
		}
		if(hasHistory)
			ImGui::EndDisabled();

		bool hasGraphEditor = m_graphEditor != nullptr;
		if(hasGraphEditor)
			ImGui::BeginDisabled();
		if(ImGui::MenuItem("Filter Graph"))
		{
			m_graphEditor = make_shared<FilterGraphEditor>(m_session, this);
			AddDialog(m_graphEditor);
		}
		if(hasGraphEditor)
			ImGui::EndDisabled();

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Window | Analyzer menu

	This menu is used for displaying protocol analyzers
 */
void MainWindow::WindowAnalyzerMenu()
{
	if(ImGui::BeginMenu("Analyzer"))
	{
		//Make a list of all filters
		auto instances = Filter::GetAllInstances();
		for(auto f : instances)
		{
			//Ignore anything that isn't a protocol decoder
			auto pd = dynamic_cast<PacketDecoder*>(f);
			if(!pd)
				continue;

			//Do we already have a dialog open for it? If so, don't make another
			if(m_protocolAnalyzerDialogs.find(pd) != m_protocolAnalyzerDialogs.end())
				continue;

			//Add it to the menu
			if(ImGui::MenuItem(pd->GetDisplayName().c_str()))
			{
				auto dlg = make_shared<ProtocolAnalyzerDialog>(pd, m_session.GetPacketManager(pd), m_session, *this);
				m_protocolAnalyzerDialogs[pd] = dlg;
				AddDialog(dlg);
			}
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Window | Generator menu

	This menu is used for connecting to a function generator that is part of an oscilloscope or other instrument.
 */
void MainWindow::WindowGeneratorMenu()
{
	if(ImGui::BeginMenu("Generator"))
	{
		auto insts = m_session.GetSCPIInstruments();
		for(auto inst : insts)
		{
			//Skip anything that's not a function generator
			if( (inst->GetInstrumentTypes() & Instrument::INST_FUNCTION) == 0)
				continue;

			//Do we already have a dialog open for it? If so, don't make another
			auto generator = dynamic_cast<SCPIFunctionGenerator*>(inst);
			if(m_generatorDialogs.find(generator) != m_generatorDialogs.end())
				continue;

			//Add it to the menu
			if(ImGui::MenuItem(generator->m_nickname.c_str()))
				m_session.AddFunctionGenerator(generator);
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Window | Multimeter menu
 */
void MainWindow::WindowMultimeterMenu()
{
	if(ImGui::BeginMenu("Multimeter"))
	{
		auto scopes = m_session.GetScopes();
		for(auto scope : scopes)
		{
			//Is the scope also a multimeter? If not, skip it
			if( (scope->GetInstrumentTypes() & Instrument::INST_DMM) == 0)
				continue;

			//Do we already have a dialog open for it? If so, don't make another
			auto meter = dynamic_cast<SCPIMultimeter*>(scope);
			if(m_meterDialogs.find(meter) != m_meterDialogs.end())
				continue;

			//Add it to the menu
			if(ImGui::MenuItem(scope->m_nickname.c_str()))
				m_session.AddMultimeter(meter);
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Runs the Window | SCPI Console menu
 */
void MainWindow::WindowSCPIConsoleMenu()
{
	if(ImGui::BeginMenu("SCPI Console"))
	{
		auto insts = m_session.GetSCPIInstruments();
		for(auto inst : insts)
		{
			//If we already have a dialog, don't show the menu
			if(m_scpiConsoleDialogs.find(inst) != m_scpiConsoleDialogs.end())
				continue;

			if(ImGui::MenuItem(inst->m_nickname.c_str()))
			{
				auto dlg = make_shared<SCPIConsoleDialog>(this, inst);
				m_scpiConsoleDialogs[inst] = dlg;
				AddDialog(dlg);
			}
		}

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Debug menu
 */
void MainWindow::DebugMenu()
{
	if(ImGui::BeginMenu("Debug"))
	{
		bool showDemo = m_showDemo;
		if(showDemo)
			ImGui::BeginDisabled();
		if(ImGui::MenuItem("ImGui Demo"))
			m_showDemo = true;
		if(showDemo)
			ImGui::EndDisabled();

		bool showPlot = m_showPlot;
		if(showPlot)
			ImGui::BeginDisabled();
		if(ImGui::MenuItem("ImPlot Demo"))
			m_showPlot = true;
		if(showPlot)
			ImGui::EndDisabled();

		ImGui::EndMenu();
	}
}

/**
	@brief Run the Help menu
 */
void MainWindow::HelpMenu()
{
	if(ImGui::BeginMenu("Help"))
	{
		ImGui::EndMenu();
	}
}
