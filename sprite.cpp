/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 * Joshua Henderson <joshua.henderson@microchip.com>
 */
#include "imagecache.h"
#include "painter.h"
#include "sprite.h"
#include "kmsscreen.h"
#include "planes/plane.h"
#include <iostream>

using namespace std;

namespace mui
{

    HardwareSprite::HardwareSprite(const std::string& filename, int framew,
				   int frameh, int framecount, int framex,
				   int framey, int x, int y)
	: PlaneWindow(Size(), FLAG_WINDOW_DEFAULT | FLAG_NO_BACKGROUND),
	  m_image(filename),
	  m_filename(filename),
	  m_frame(framew,frameh),
	  m_index(0)
    {
	m_strip = add_strip(framecount,framex,framey);

	add(&m_image);

	resize(m_image.w(), m_image.h());

	KMSOverlayScreen* s = reinterpret_cast<KMSOverlayScreen*>(screen());
	plane_set_pan_pos(s->s(), m_strips[m_strip].framex, m_strips[m_strip].framey);
	plane_set_pan_size(s->s(), m_frame.w, m_frame.h);
	//plane_apply(s->s());

	// hack to change the size because the screen size and the box size are different
	position(x,y);
	m_box = Rect(x, y, framew, frameh);

	damage();
    }

    bool HardwareSprite::is_last_frame() const
    {
	return m_index >= m_strips[m_strip].framecount-1;
    }

    void HardwareSprite::advance()
    {
	int index = m_index;
	if (++index >= m_strips[m_strip].framecount)
	    index = 0;

	show_frame(index);
    }

    void HardwareSprite::show_frame(int index)
    {
	if (index != m_index)
	{
	    m_index = index;

	    int imagew = m_image.w();
	    int panx = m_strips[m_strip].framex + (m_index * m_frame.w);
	    int pany = m_strips[m_strip].framey;

	    // support sheets that have frames on multiple rows
	    if (panx + m_frame.w >= imagew)
	    {
		int x = m_strips[m_strip].framex + (m_index * m_frame.w);

		panx = (x % imagew);
		pany = ((x / imagew) * m_strips[m_strip].framey) + (x / imagew) * m_frame.h;
	    }

	    KMSOverlayScreen* s = reinterpret_cast<KMSOverlayScreen*>(screen());

	    plane_set_pan_pos(s->s(), panx, pany);
	    plane_set_pan_size(s->s(), m_frame.w, m_frame.h);
	    plane_apply(s->s());
	}
    }

    shared_cairo_surface_t HardwareSprite::surface() const
    {
	int imagew = m_image.w();
	int panx = m_strips[m_strip].framex + (m_index * m_frame.w);
	int pany = m_strips[m_strip].framey;

	// support sheets that have frames on multiple rows
	if (panx + m_frame.w >= imagew)
	{
	    int x = m_strips[m_strip].framex + (m_index * m_frame.w);

	    panx = (x % imagew);
	    pany = ((x / imagew) * m_strips[m_strip].framey) + (x / imagew) * m_frame.h;
	}


	// cairo_surface_create_for_rectangle() would work here with one
	// exception - the resulting image has no width and height

	shared_cairo_surface_t copy = shared_cairo_surface_t(cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
												m_frame.w,
												m_frame.h),
							     cairo_surface_destroy);

	shared_cairo_t cr = shared_cairo_t(cairo_create(copy.get()), cairo_destroy);
	cairo_set_source_surface(cr.get(), m_image.surface().get(), -panx, -pany);
	cairo_rectangle(cr.get(), 0, 0, m_frame.w, m_frame.h);
	cairo_set_operator(cr.get(), CAIRO_OPERATOR_SOURCE);
	cairo_fill(cr.get());

	return copy;
    }

    HardwareSprite::~HardwareSprite()
    {

    }


    SoftwareSprite::SoftwareSprite(const std::string& filename, int framew, int frameh,
				   int framecount, int framex, int framey,
				   int x, int y)
	: Widget(x, y, framew, frameh),
	  m_filename(filename),
	  m_framex(framex),
	  m_framey(framey),
	  m_frame(framew,frameh),
	  m_index(0),
	  m_framecount(framecount)
    {
	m_image = image_cache.get(filename, 1.0);
	assert(m_image.get());

	m_box = Rect(x, y, framew, frameh);

	//show_frame(0);
    }

    void SoftwareSprite::draw(const Rect& rect)
    {
	int imagew = cairo_image_surface_get_width(m_image.get());

	int panx = m_framex + (m_index * m_frame.w);
	int pany = m_framey;

	// support sheets that have frames on multiple rows
	if (panx + m_frame.w >= imagew)
	{
	    int x = m_framex + (m_index * m_frame.w);

	    panx = (x % imagew);
	    pany = ((x / imagew) * m_framey) + (x / imagew) * m_frame.h;
	}

	Painter painter(screen()->context());
	painter.draw_image(Rect(panx, pany, m_frame.w, m_frame.h), box().point(), m_image);
    }

    void SoftwareSprite::advance()
    {
	int index = m_index;
	if (++index >= m_framecount)
	    index = 0;

	show_frame(index);
    }

    void SoftwareSprite::show_frame(int index)
    {
	if (index != m_index)
	{
	    m_index = index;
	    damage();
	}
    }

    SoftwareSprite::~SoftwareSprite()
    {
    }

}
