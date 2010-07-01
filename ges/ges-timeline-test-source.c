/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *               2009 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:ges-timeline-test-source
 * @short_description: Render video and audio test patterns in a
 * #GESTimelineLayer
 * 
 * Useful for testing purposes or for filling gaps between media in
 * a #GESTimelineLayer.
 */

#include "ges-internal.h"
#include "ges-timeline-test-source.h"
#include "ges-timeline-source.h"
#include "ges-track-object.h"
#include "ges-track-video-test-source.h"
#include "ges-track-audio-test-source.h"
#include <string.h>

G_DEFINE_TYPE (GESTimelineTestSource, ges_timeline_test_source,
    GES_TYPE_TIMELINE_SOURCE);

#define GES_TIMELINE_TEST_SOURCE_VPATTERN_TYPE\
  ges_timeline_test_source_vpattern_get_type()

/* table more-or-less copied from gstvideotestsrc.c */
static GEnumValue vpattern_enum_values[] = {
  {GES_TRACK_VIDEO_BG_SRC_SMPTE, "SMPTE 100% color bars", "smpte"},
  {GES_TRACK_VIDEO_BG_SRC_SNOW, "Random (television snow)", "snow"},
  {GES_TRACK_VIDEO_BG_SRC_BLACK, "100% Black", "black"},
  {GES_TRACK_VIDEO_BG_SRC_WHITE, "100% White", "white"},
  {GES_TRACK_VIDEO_BG_SRC_RED, "Red", "red"},
  {GES_TRACK_VIDEO_BG_SRC_GREEN, "Green", "green"},
  {GES_TRACK_VIDEO_BG_SRC_BLUE, "Blue", "blue"},
  {GES_TRACK_VIDEO_BG_SRC_CHECKERS1, "Checkers 1px", "checkers-1"},
  {GES_TRACK_VIDEO_BG_SRC_CHECKERS2, "Checkers 2px", "checkers-2"},
  {GES_TRACK_VIDEO_BG_SRC_CHECKERS4, "Checkers 4px", "checkers-4"},
  {GES_TRACK_VIDEO_BG_SRC_CHECKERS8, "Checkers 8px", "checkers-8"},
  {GES_TRACK_VIDEO_BG_SRC_CIRCULAR, "Circular", "circular"},
  {GES_TRACK_VIDEO_BG_SRC_BLINK, "Blink", "blink"},
  {GES_TRACK_VIDEO_BG_SRC_SMPTE75, "SMPTE 75% color bars", "smpte75"},
  {0, NULL, NULL}
};

GType ges_timeline_test_source_vpattern_get_type (void);

GType
ges_timeline_test_source_vpattern_get_type (void)
{

  static gsize once = 0;
  static GType theType = 0;

  if (g_once_init_enter (&once)) {
    theType = g_enum_register_static ("GESTimelineTestSourceVPattern",
        vpattern_enum_values);
    g_once_init_leave (&once, 1);
  };

  return theType;
}

enum
{
  PROP_0,
  PROP_MUTE,
  PROP_VPATTERN,
};

static void
ges_timeline_test_source_set_mute (GESTimelineTestSource * self, gboolean mute);

static void
ges_timeline_test_source_set_vpattern (GESTimelineTestSource * self,
    gint vpattern);

static GESTrackObject
    * ges_timeline_test_source_create_track_object (GESTimelineObject * obj,
    GESTrack * track);

static void
ges_timeline_test_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESTimelineTestSource *tfs = GES_TIMELINE_TEST_SOURCE (object);

  switch (property_id) {
    case PROP_MUTE:
      g_value_set_boolean (value, tfs->mute);
      break;
    case PROP_VPATTERN:
      g_value_set_enum (value, tfs->vpattern);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_timeline_test_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESTimelineTestSource *tfs = GES_TIMELINE_TEST_SOURCE (object);

  switch (property_id) {
    case PROP_MUTE:
      ges_timeline_test_source_set_mute (tfs, g_value_get_boolean (value));
      break;
    case PROP_VPATTERN:
      ges_timeline_test_source_set_vpattern (tfs, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_timeline_test_source_dispose (GObject * object)
{
  G_OBJECT_CLASS (ges_timeline_test_source_parent_class)->dispose (object);
}

static void
ges_timeline_test_source_finalize (GObject * object)
{
  G_OBJECT_CLASS (ges_timeline_test_source_parent_class)->finalize (object);
}

static void
ges_timeline_test_source_class_init (GESTimelineTestSourceClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESTimelineObjectClass *timobj_class = GES_TIMELINE_OBJECT_CLASS (klass);

  object_class->get_property = ges_timeline_test_source_get_property;
  object_class->set_property = ges_timeline_test_source_set_property;
  object_class->dispose = ges_timeline_test_source_dispose;
  object_class->finalize = ges_timeline_test_source_finalize;

  /**
   * GESTimelineTestSource:vpattern:
   *
   * Video pattern to display in video track objects.
   */
  g_object_class_install_property (object_class, PROP_VPATTERN,
      g_param_spec_enum ("vpattern", "VPattern",
          "Which video pattern to display. See videotestsrc element",
          GES_TIMELINE_TEST_SOURCE_VPATTERN_TYPE,
          GES_TRACK_VIDEO_BG_SRC_BLACK, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * GESTimelineTestSource:mute:
   *
   * Whether the sound will be played or not.
   */
  g_object_class_install_property (object_class, PROP_MUTE,
      g_param_spec_boolean ("mute", "Mute", "Mute audio track",
          FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  timobj_class->create_track_object =
      ges_timeline_test_source_create_track_object;
  timobj_class->need_fill_track = FALSE;
}

static void
ges_timeline_test_source_init (GESTimelineTestSource * self)
{
  GES_TIMELINE_OBJECT (self)->duration = 0;
}

static void
ges_timeline_test_source_set_mute (GESTimelineTestSource * self, gboolean mute)
{
  GList *tmp;
  GESTimelineObject *object = (GESTimelineObject *) self;

  GST_DEBUG ("self:%p, mute:%d", self, mute);

  self->mute = mute;

  /* Go over tracked objects, and update 'active' status on all audio objects */
  for (tmp = object->trackobjects; tmp; tmp = tmp->next) {
    GESTrackObject *trackobject = (GESTrackObject *) tmp->data;

    if (trackobject->track->type == GES_TRACK_TYPE_AUDIO)
      ges_track_object_set_active (trackobject, !mute);
  }
}

static void
ges_timeline_test_source_set_vpattern (GESTimelineTestSource * self,
    gint vpattern)
{
  GList *tmp;
  GESTimelineObject *object = (GESTimelineObject *) self;

  self->vpattern = vpattern;

  for (tmp = object->trackobjects; tmp; tmp = tmp->next) {
    GESTrackObject *trackobject = (GESTrackObject *) tmp->data;
    if (GES_IS_TRACK_VIDEO_TEST_SOURCE (trackobject))
      ges_track_video_test_source_set_pattern (
          (GESTrackVideoTestSource *) trackobject, vpattern);
  }
}

static GESTrackObject *
ges_timeline_test_source_create_track_object (GESTimelineObject * obj,
    GESTrack * track)
{
  GESTimelineTestSource *tfs = (GESTimelineTestSource *) obj;
  GESTrackObject *res = NULL;

  GST_DEBUG ("Creating a GESTrackTestSource");

  if (track->type == GES_TRACK_TYPE_VIDEO) {
    res = (GESTrackObject *) ges_track_video_test_source_new ();
    ges_track_video_test_source_set_pattern (
        (GESTrackVideoTestSource *) res, tfs->vpattern);
  }

  else if (track->type == GES_TRACK_TYPE_AUDIO) {
    res = (GESTrackObject *) ges_track_audio_test_source_new ();
    if (tfs->mute)
      ges_track_object_set_active (res, FALSE);
  }

  else
    res = NULL;

  return res;
}

/**
 * ges_timeline_testsource_new:
 * @uri: the URI the source should control
 *
 * Creates a new #GESTimelineTestSource for the provided @uri.
 *
 * Returns: The newly created #GESTimelineTestSource, or NULL if there was an
 * error.
 */
GESTimelineTestSource *
ges_timeline_test_source_new (void)
{
  /* FIXME : Check for validity/existence of URI */
  return g_object_new (GES_TYPE_TIMELINE_TEST_SOURCE, NULL);
}

GESTimelineTestSource *
ges_timeline_test_source_new_for_nick (gchar * nick)
{
  GESTimelineTestSource *ret;
  GEnumValue *value;
  int i;

  for (i = 0, value = &vpattern_enum_values[i]; value->value_nick;
      value = &vpattern_enum_values[i++]) {
    if (!strcmp (nick, value->value_nick)) {
      ret = g_object_new (GES_TYPE_TIMELINE_TEST_SOURCE, "vpattern",
          (gint) value->value, NULL);
      return ret;
    }
    value++;
  }

  return NULL;
}
