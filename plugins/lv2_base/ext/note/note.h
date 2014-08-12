/*
  Copyright 2014 Hannu Haahti <grejppi@gmail.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file note.h C header for the LV2 Note extension
   <http://grejppi.github.io/ns/ext/note>.

   The note extension is purely data, this header merely defines URIs
   for convenience.
*/

#ifndef LV2_NOTE_H
#define LV2_NOTE_H

#define LV2_NOTE_URI    "http://grejppi.github.io/ns/ext/note"
#define LV2_NOTE_PREFIX LV2_NOTE_URI "#"

#define LV2_NOTE__NoteEvent     LV2_NOTE_PREFIX "NoteEvent"
#define LV2_NOTE__id            LV2_NOTE_PREFIX "id"
#define LV2_NOTE__gate          LV2_NOTE_PREFIX "gate"
#define LV2_NOTE__frequency     LV2_NOTE_PREFIX "frequency"
#define LV2_NOTE__velocity      LV2_NOTE_PREFIX "velocity"
#define LV2_NOTE__stereoPanning LV2_NOTE_PREFIX "stereoPanning"
#define LV2_NOTE__expects       LV2_NOTE_PREFIX "expects"

#endif  /* LV2_NOTE_H */
