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

#ifndef Timeline_h
#define Timeline_h

class WaveformGroup;
class OscilloscopeWindow;

class Timeline : public Gtk::Layout
{
public:
	Timeline(OscilloscopeWindow* parent, WaveformGroup* group);
	virtual ~Timeline();

	void RefreshUnits();

	Unit GetXAxisUnits()
	{ return m_xAxisUnit; }

	bool IsDraggingTrigger()
	{ return (m_dragState == DRAG_PRIMARY_TRIGGER) || (m_dragState == DRAG_SECONDARY_TRIGGER); }

	int64_t GetTriggerDragPosition()
	{ return m_currentTriggerOffsetDragPosition; }

protected:

	enum DragState
	{
		DRAG_NONE,
		DRAG_TIMELINE,
		DRAG_PRIMARY_TRIGGER,
		DRAG_SECONDARY_TRIGGER,
	} m_dragState;

	double m_dragStartX;
	int64_t m_originalTimeOffset;
	int64_t m_currentTriggerOffsetDragPosition;

	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
	virtual bool on_button_press_event(GdkEventButton* event);
	virtual bool on_button_release_event(GdkEventButton* event);
	virtual bool on_motion_notify_event(GdkEventMotion* event);
	virtual bool on_scroll_event (GdkEventScroll* ev);

	void RenderTriggerArrow(
		const Cairo::RefPtr<Cairo::Context>& cr,
		Oscilloscope* scope,
		float xscale,
		size_t h
		);

	DragState HitTest(double x, double y, Oscilloscope** pscope = NULL);

	virtual void on_realize();

	void Render(const Cairo::RefPtr<Cairo::Context>& cr, OscilloscopeChannel* chan);

	virtual void DrawCursor(
		const Cairo::RefPtr<Cairo::Context>& cr,
		int64_t fs,
		const char* name,
		Gdk::Color color,
		bool draw_left,
		bool show_delta);

	virtual void DrawMarker(
		const Cairo::RefPtr<Cairo::Context>& cr,
		int64_t fs,
		std::string& name,
		Gdk::Color color);

	WaveformGroup* m_group;
	OscilloscopeWindow* m_parent;

	float GetDPIScale()
	{ return get_pango_context()->get_resolution() / 96; }

	Unit m_xAxisUnit;

	Oscilloscope* m_dragScope;
};

#endif
