// SPDX-License-Identifier: GPL-3.0-or-later
/* Independently installed calendar. No Calendar application ABI is exported. */
#include <infiltratr/core.h>
#include <infiltratr/temporal_calendar.h>
#include <glib.h>
extern "C" {
#include "calendar-engine-types.h"
#include "calendar-arithmetic.h"
#include "calendar-custom.h"
#include "calendar-historical.h"
#include "calendar-swedish.h"
#include "icu-calendar.h"
#include "catalogue.h"
#include "julian-day.h"
}
#include <string.h>

static gboolean
swedish_fields(gint64 jdn,
               CalendarCalendarFields *fields)
{
    if (jdn < calendar_julian_to_jdn(G_MININT, 1, 1) ||
        jdn > calendar_gregorian_to_jdn(G_MAXINT, 12, 31))
        return FALSE;
    calendar_swedish_fields_from_jdn(jdn, fields);
    return TRUE;
}

static gchar *
swedish_format(gint64 jdn,
               CalendarDatePart part)
{
    CalendarCalendarFields fields;

    calendar_swedish_fields_from_jdn(jdn, &fields);
    return calendar_swedish_format(&fields, part);
}

static bool fields(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn, InfiltratrTemporalCalendarFields *result)
{
    CalendarCalendarFields value;
    (void)self;
    if (result == NULL || !swedish_fields(jdn,&value))
        return false;
    *result = InfiltratrTemporalCalendarFields{
        value.year, value.month, value.day, value.auxiliary, value.special != FALSE };
    return true;
}
static bool format(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn, InfiltratrTemporalCalendarDatePart part,
    char *buffer, size_t capacity, size_t *length)
{
    gchar *text;
    CalendarCalendarFields validated_fields;
    (void)self;
    if (buffer == NULL || capacity == 0U ||
        part < INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_DAY ||
        part > INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_FULL ||
        !swedish_fields( jdn, &validated_fields))
        return false;
    /* Field conversion owns supported chronology. Formatting must not ask
     * ICU to invent dates outside a bounded arithmetic provider's range. */
    text = swedish_format(jdn, (CalendarDatePart)part);
    if (text == NULL || *text == '\0') { g_free(text); return false; }
    const size_t size = strlen(text);
    if (size >= capacity) { g_free(text); return false; }
    memcpy(buffer,text,size+1U);
    if (length != NULL) *length=size;
    g_free(text);
    return true;
}
static bool start(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn, int64_t *result)
{
    CalendarCalendarFields value;
    InfiltratrTemporalCalendarFields checked{};
    if (result == nullptr || !fields(self,jdn,&checked))
        return false;
    const int64_t candidate=calendar_swedish_month_start(jdn);
    if (!swedish_fields(candidate,&value)) return false;
    *result=candidate;
    return true;
}
static bool navigate(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn,int32_t amount,int64_t *result,bool by_year)
{
    CalendarCalendarFields value;
    InfiltratrTemporalCalendarFields checked{};
    if (result == nullptr || !fields(self,jdn,&checked))
        return false;
    const int64_t candidate=by_year
        ? calendar_swedish_add_years(jdn,amount)
        : calendar_swedish_add_months(jdn,amount);
    if (!swedish_fields(candidate,&value)) return false;
    *result=candidate;
    return true;
}
static bool periods(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn,int32_t amount,int64_t *result)
{ return navigate(self,jdn,amount,result,false); }
static bool years(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn,int32_t amount,int64_t *result)
{ return navigate(self,jdn,amount,result,true); }
static const InfiltratrTemporalResourceMetadata metadata = {
    sizeof(InfiltratrTemporalResourceMetadata),
    INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,RESOURCE_ID,INFILTRATR_TEMPORAL_DISPLAY_NAME,false,false,false
};
static const InfiltratrTemporalCalendarProvider provider = {
    sizeof(InfiltratrTemporalCalendarProvider),
    INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI,RESOURCE_ID,
    fields,format,start,periods,years
};
#if defined(_WIN32)
#define RESOURCE_EXPORT __declspec(dllexport)
#else
#define RESOURCE_EXPORT __attribute__((visibility("default")))
#endif
extern "C" {
RESOURCE_EXPORT const InfiltratrTemporalResourceMetadata *infiltratr_temporal_resource_metadata_v1(void);
RESOURCE_EXPORT const InfiltratrTemporalCalendarProvider *infiltratr_temporal_calendar_provider_v1(void);
RESOURCE_EXPORT const InfiltratrTemporalResourceMetadata *infiltratr_temporal_resource_metadata_v1(void) { return &metadata; }
RESOURCE_EXPORT const InfiltratrTemporalCalendarProvider *infiltratr_temporal_calendar_provider_v1(void) { return &provider; }
}
