#ifndef __RICHEPG_JSON_H__
#define __RICHEPG_JSON_H__
#include <time.h>
#include <gxtype.h>

#define RICHEPG_MASTERINDEX_VERSION_DEF 2
#define RICHEPG_LOCALE_LANGUAGE_LEN     3
#define RICHEPG_LOCALE_LANGUAGE_DEF     "eng"
#define RICHEPG_FILE_TYPE_NUM           7

enum richepg_file_type
{
    RICHEPG_NULL        = 0,
    RICHEPG_MASTERINDEX = 1 << 0,
    RICHEPG_ELLI        = 1 << 1,
    RICHEPG_ECF         = 1 << 2,
    RICHEPG_ELLD        = 1 << 3,
    RICHEPG_ECL         = 1 << 4,
    RICHEPG_EPG         = 1 << 5,
    RICHEPG_EAR         = 1 << 6
};

struct eutelsat_masterindex
{
    bool got;

    int syntaxVersion;
    time_t lastUpdated;

    unsigned char ear_num;
    struct eutelsat_resource
    {
        time_t lastUpdated;
        unsigned short int componentTag;
        char *path;
    } ecf, ecl, ear, elli;

    unsigned char elld_num;
    struct eutelsat_masterindex_elld
    {
        time_t lastUpdated;
        unsigned int lineup;
        unsigned short int componentTag;
        char *path;
    } *elld;

    unsigned int epg_num;
    struct eutelsat_masterindex_epg
    {
        time_t lastUpdated;
        unsigned int duration;
        time_t startDate;
        unsigned short int componentTag;
        char *path;
    } *epg;
};

struct eutelsat_locale
{
    char lang[RICHEPG_LOCALE_LANGUAGE_LEN];
    char *name;
    char *description;
    bool is_default;
};

struct eutelsat_configuration_file
{
    bool got;

    unsigned int ott_num;
    struct eutelsat_ott_backend
    {
        char *type;
        int id;
        char *endPointURL;
        char *tenantId;
        char *drmEndPointURL;
        char *drmTenantId;
    } *ott;

    unsigned int service_num;
    struct eutelsat_service_type
    {
        int id;
        char *label;
    } *service;

    unsigned int channel_num;
    unsigned int content_num;
    struct eutelsat_genre
    {
        int id;
        unsigned int locales_num;
        struct eutelsat_locale *locales;
    } *channel, *content;

    handle_t channel_hashmap;
    handle_t content_hashmap;

    /*
     *  tier_num: number of tiers
     *  id: tier id
     *  name: tier name
     *  channelLogo:
     *      true: do not store channel logo
     *  epgLogo:
     *      true: do not store event logo
     *  epgDesc:
     *      true: do not store event description
     *  epgDuration: epg min duration time in seconds
     *
     */
    unsigned int tier_num;
    struct eutelsat_tier
    {
        int id;
        //char *name;
        bool channelLogo;
        bool epgLogo;
        bool epgDesc;
        unsigned int epgDuration;
    } *tiers;

    handle_t tiers_hashmap;
};

enum
{
    EUTELSAT_LOGO_INVALID,
    EUTELSAT_LOGO_PNG,
    EUTELSAT_LOGO_JPG
};

enum
{
    EUTELSAT_DELIVERY_INVALID,
    EUTELSAT_DELIVERY_DVB,
    EUTELSAT_DELIVERY_OTT,
    EUTELSAT_DELIVERY_HYBRID
};

struct eutelsat_logo
{
    unsigned char type;
    unsigned int width;
    unsigned int height;
    unsigned char delivery;
    char *path;
    char *url;
};

struct eutelsat_lineup_list
{
    unsigned int lineup_num;
    bool got;
    unsigned short int component_tag;

    struct eutelsat_elli
    {
        int id;
        int wakeupchannelid;

        struct eutelsat_lineup_customer
        {
            char *phone;
            char *url;
        } customer;

        unsigned int logos_num;
        struct eutelsat_logo *logos;

        unsigned int locales_num;
        struct eutelsat_locale *locales;
    } *lineups;
};

struct eutelsat_lineup_data
{
    bool got;
    unsigned short int component_tag;

    unsigned int lineup_id;

    unsigned int channel_num;
    struct eutelsat_lineup_channel
    {
        int id;
        unsigned int lcn_num;
        int *lcns;
        unsigned int genres_num;
        int *genres;
        int tier;
    } *channels;
    handle_t hashmap;
};

struct eutelsat_channel_list
{
    bool got;
    unsigned short int component_tag;

    unsigned int channels_num;
    struct eutelsat_channel
    {
        int id;
        bool scrambled;

        unsigned int logos_num;
        struct eutelsat_logo *logos;

        unsigned int locales_num;
        struct eutelsat_locale *locales;

        unsigned int tech_num;
        struct eutelsat_techchannel
        {
            int id;

            unsigned int services_num;
            int *services;

            unsigned char delivery;
            union
            {
                struct eutelsat_techchannel_dvb
                {
                    int onid;
                    int tsid;
                    int sid;
                } dvb;

                struct eutelsat_techchannel_ott
                {
                    int backendid;
                    int channelid;
                } ott;
            } trans;
        } *techs;
    } *channels;
    handle_t hashmap;
};

struct eutelsat_epg
{
    unsigned short int component_tag;
    unsigned int channels_num;
    struct eutelsat_epg_channel
    {
        unsigned char need_save;
        unsigned int ids_num;
        int *ids;
        unsigned int events_num;
        struct eutelsat_epg_event
        {
            unsigned char need_save;
            int id;
            int duration;
            time_t starttime;
            unsigned int contents_num;
            int *contents;
            int parental;
            unsigned int logos_num;
            struct eutelsat_logo *logos;

            unsigned int locales_num;
            struct eutelsat_locale *locales;
        } *events;
    } *channels;
    handle_t hashmap;
};

struct eutelsat_additional_resources
{
    bool got;
    unsigned short int component_tag;
    int lineup_id;

    int id;
    unsigned int g_logos_num;
    struct eutelsat_logo *g_logos;
    unsigned int channels_num;
    struct ear_channel
    {
        int id;
        unsigned int logos_num;
        struct eutelsat_logo *logos;
    } *channels;
    handle_t hashmap;
};

struct richepg_channel_tiers {
    bool enable;
    bool channelLogo;
    bool epgLogo;
    bool epgDesc;
    unsigned int epgDuration;
    unsigned int epgDescDuration;
    unsigned int maxEpgDuration;
};

void richepg_set_channel_tiers(struct richepg_channel_tiers *tiers);
struct richepg_channel_tiers *richepg_get_channel_tiers(void);

int richepg_free_masterindex(struct eutelsat_masterindex *masterindex);
int richepg_parse_masterindex(bool from_file, uint8_t *data, struct eutelsat_masterindex *masterindex);

int richepg_free_ecf(struct eutelsat_configuration_file *ecf);
int richepg_parse_ecf(bool from_file, uint8_t *data, struct eutelsat_configuration_file *ecf);

int richepg_free_elli(struct eutelsat_lineup_list *elli);
int richepg_parse_elli(bool from_file, uint8_t *data, struct eutelsat_lineup_list *elli);

int richepg_free_elld(struct eutelsat_lineup_data *elld);
int richepg_parse_elld(bool from_file, uint8_t *data, struct eutelsat_lineup_data *elld);
int richepg_parse_mini_elld(bool from_file, uint8_t *data, struct eutelsat_lineup_data *elld);
int richepg_save_mini_elld(const char *path, struct eutelsat_lineup_data *elld);

int richepg_free_ecl(struct eutelsat_channel_list *ecl);
int richepg_parse_ecl(bool from_file, uint8_t *data, struct eutelsat_channel_list *ecl);

int richepg_free_ear(struct eutelsat_additional_resources *ear);
int richepg_parse_ear(bool from_file, uint8_t *data, uint32_t lineup_id, struct eutelsat_additional_resources *ear);

int richepg_free_epg_event(struct eutelsat_epg_event *event);
int richepg_free_epg(struct eutelsat_epg *epg);
int richepg_parse_epg(bool enable_tier, struct eutelsat_lineup_data *elld, struct eutelsat_configuration_file *ecf, bool from_file, uint8_t *data, struct eutelsat_epg *epg);

int richepg_get_locale_index(uint32_t total, struct eutelsat_locale *locales);
int richepg_get_logo_index(uint32_t total, struct eutelsat_logo *logos);

int richepg_epg_get_event_from_json(char *filename, int channel_id, int event_id, struct eutelsat_epg_event *epg_event, uint16_t component_tag);
#endif
