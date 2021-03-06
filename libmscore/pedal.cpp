//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "pedal.h"
#include "sym.h"
#include "xml.h"
#include "system.h"
#include "measure.h"
#include "chordrest.h"
#include "staff.h"

#include "score.h"

namespace Ms {

static const ElementStyle pedalStyle {
      { Sid::pedalFontFace,                      Pid::BEGIN_FONT_FACE         },
      { Sid::pedalFontFace,                      Pid::CONTINUE_FONT_FACE      },
      { Sid::pedalFontFace,                      Pid::END_FONT_FACE           },
      { Sid::pedalFontSize,                      Pid::BEGIN_FONT_SIZE         },
      { Sid::pedalFontSize,                      Pid::CONTINUE_FONT_SIZE      },
      { Sid::pedalFontSize,                      Pid::END_FONT_SIZE           },
      { Sid::pedalFontBold,                      Pid::BEGIN_FONT_BOLD         },
      { Sid::pedalFontBold,                      Pid::CONTINUE_FONT_BOLD      },
      { Sid::pedalFontBold,                      Pid::END_FONT_BOLD           },
      { Sid::pedalFontItalic,                    Pid::BEGIN_FONT_ITALIC       },
      { Sid::pedalFontItalic,                    Pid::CONTINUE_FONT_ITALIC    },
      { Sid::pedalFontItalic,                    Pid::END_FONT_ITALIC         },
      { Sid::pedalFontUnderline,                 Pid::BEGIN_FONT_UNDERLINE    },
      { Sid::pedalFontUnderline,                 Pid::CONTINUE_FONT_UNDERLINE },
      { Sid::pedalFontUnderline,                 Pid::END_FONT_UNDERLINE      },
      { Sid::pedalTextAlign,                     Pid::BEGIN_TEXT_ALIGN        },
      { Sid::pedalTextAlign,                     Pid::CONTINUE_TEXT_ALIGN     },
      { Sid::pedalTextAlign,                     Pid::END_TEXT_ALIGN          },
      { Sid::pedalHookHeight,                    Pid::BEGIN_HOOK_HEIGHT       },
      { Sid::pedalHookHeight,                    Pid::END_HOOK_HEIGHT         },
      { Sid::pedalBeginTextOffset,               Pid::BEGIN_TEXT_OFFSET       },
      { Sid::pedalBeginTextOffset,               Pid::CONTINUE_TEXT_OFFSET    },
      { Sid::pedalBeginTextOffset,               Pid::END_TEXT_OFFSET         },
      { Sid::pedalPlacement,                     Pid::PLACEMENT               },
      };

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void PedalSegment::layout()
      {
      TextLineBaseSegment::layout();
      autoplaceSpannerSegment(spatium() * .7, Sid::pedalPosBelow, Sid::pedalPosAbove);
      }


//---------------------------------------------------------
//   Pedal
//---------------------------------------------------------

Pedal::Pedal(Score* s)
   : TextLineBase(s)
      {
      initElementStyle(&pedalStyle);
      setLineVisible(true);
      resetProperty(Pid::BEGIN_TEXT);
      resetProperty(Pid::END_TEXT);

      resetProperty(Pid::LINE_WIDTH);
      resetProperty(Pid::LINE_STYLE);

      resetProperty(Pid::BEGIN_HOOK_TYPE);
      resetProperty(Pid::END_HOOK_TYPE);

      resetProperty(Pid::BEGIN_TEXT_PLACE);
      resetProperty(Pid::LINE_VISIBLE);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Pedal::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            if (!TextLineBase::readProperties(e))
                  e.unknown();
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Pedal::write(XmlWriter& xml) const
      {
      if (!xml.canWrite(this))
            return;
      xml.stag(name());

      for (auto i : {
         Pid::END_HOOK_TYPE,
         Pid::BEGIN_TEXT,
         Pid::END_TEXT,
         Pid::LINE_WIDTH,
         Pid::LINE_STYLE,
         Pid::BEGIN_HOOK_TYPE
         }) {
            writeProperty(xml, i);
            }
      for (const StyledProperty& spp : *styledProperties())
            writeProperty(xml, spp.pid);

      Element::writeProperties(xml);
      xml.etag();
      }


//---------------------------------------------------------
//   createLineSegment
//---------------------------------------------------------

LineSegment* Pedal::createLineSegment()
      {
      return new PedalSegment(score());
      }

//---------------------------------------------------------
//   setYoff
//---------------------------------------------------------

void Pedal::setYoff(qreal val)
      {
      rUserYoffset() += val * spatium() - score()->styleP(placeAbove() ? Sid::pedalPosAbove : Sid::pedalPosBelow);
      }

//---------------------------------------------------------
//   propertyDefault
//---------------------------------------------------------

QVariant Pedal::propertyDefault(Pid propertyId) const
      {
      switch (propertyId) {
            case Pid::LINE_WIDTH:
                  return score()->styleP(Sid::pedalLineWidth);    // return point, not spatium

            case Pid::LINE_STYLE:
                  return score()->styleV(Sid::pedalLineStyle);

            case Pid::BEGIN_TEXT:
            case Pid::CONTINUE_TEXT:
            case Pid::END_TEXT:
                  return "";

            case Pid::BEGIN_TEXT_PLACE:
            case Pid::CONTINUE_TEXT_PLACE:
            case Pid::END_TEXT_PLACE:
                  return int(PlaceText::LEFT);

            case Pid::BEGIN_HOOK_TYPE:
            case Pid::END_HOOK_TYPE:
                  return int(HookType::NONE);

            case Pid::LINE_VISIBLE:
                  return true;

            default:
                  return TextLineBase::propertyDefault(propertyId);
            }
      }

//---------------------------------------------------------
//   linePos
//    return System() coordinates
//---------------------------------------------------------

QPointF Pedal::linePos(Grip grip, System** sys) const
      {
      qreal x = 0.0;
      qreal nhw = score()->noteHeadWidth();
      System* s = nullptr;
      if (grip == Grip::START) {
            ChordRest* c = toChordRest(startElement());
            if (c) {
                  s = c->segment()->system();
                  x = c->pos().x() + c->segment()->pos().x() + c->segment()->measure()->pos().x();
                  if (c->type() == ElementType::REST && c->durationType() == TDuration::DurationType::V_MEASURE)
                        x -= c->x();
                  if (beginHookType() == HookType::HOOK_45)
                        x += nhw * .5;
                  }
            }
      else {
            Element* e = endElement();
            ChordRest* c = toChordRest(endElement());
            if (!e || e == startElement() || (endHookType() == HookType::HOOK_90)) {
                  // pedal marking on single note or ends with non-angled hook:
                  // extend to next note or end of measure
                  Segment* seg = nullptr;
                  if (!e)
                        seg = startSegment();
                  else
                        seg = c->segment();
                  if (seg) {
                        seg = seg->next();
                        for ( ; seg; seg = seg->next()) {
                              if (seg->segmentType() == SegmentType::ChordRest) {
                                    // look for a chord/rest in any voice on this staff
                                    bool crFound = false;
                                    int track = staffIdx() * VOICES;
                                    for (int i = 0; i < VOICES; ++i) {
                                          if (seg->element(track + i)) {
                                                crFound = true;
                                                break;
                                                }
                                          }
                                    if (crFound)
                                          break;
                                    }
                              else if (seg->segmentType() == SegmentType::EndBarLine) {
                                    break;
                                    }
                              }
                        }
                  if (seg) {
                        s = seg->system();
                        x = seg->pos().x() + seg->measure()->pos().x() - nhw * 2;
                        }
                  }
            else if (c) {
                  s = c->segment()->system();
                  x = c->pos().x() + c->segment()->pos().x() + c->segment()->measure()->pos().x();
                  if (c->type() == ElementType::REST && c->durationType() == TDuration::DurationType::V_MEASURE)
                        x -= c->x();
                  }
            if (!s) {
                  int t = tick2();
                  Measure* m = score()->tick2measure(t);
                  s = m->system();
                  x = m->tick2pos(t);
                  }
            if (endHookType() == HookType::HOOK_45)
                  x += nhw * .5;
            else
                  x += nhw;
            }

      *sys = s;
      return QPointF(x, 0);
      }

}

