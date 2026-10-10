// NOLINTBEGIN
#pragma once

// Models of three benchmark datasets for reading JSON into and writing it from C++ types.
//
// The same aggregates are read and written by cpp-yyjson, through its field-name
// reflection, and by Glaze, through its own, so both libraries fill the same objects.
// Members are declared in the order their keys appear in the documents, which is also
// the order both libraries write them in.  Every key of the documents has a member,
// except `protected` of a Twitter user, which is a C++ keyword: neither library can
// rename a reflected member, so both skip that key as unknown when reading and write the
// member as `protected_`.  Nothing else is skipped, so a library that is fast at skipping
// values gains little here.
//
// A key whose value is always null in the document is a `std::optional`, as is a key
// that only some of the objects have.  Both libraries leave out an empty optional when
// writing, so they write the same keys.

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bench_typed
{
    // canada.json: a GeoJSON polygon of 111 thousand coordinate pairs
    namespace canada
    {
        struct properties_t
        {
            std::string name;
            bool operator==(const properties_t&) const = default;
        };

        struct geometry_t
        {
            std::string type;
            std::vector<std::vector<std::array<double, 2>>> coordinates;
            bool operator==(const geometry_t&) const = default;
        };

        struct feature_t
        {
            std::string type;
            properties_t properties;
            geometry_t geometry;
            bool operator==(const feature_t&) const = default;
        };

        struct document
        {
            std::string type;
            std::vector<feature_t> features;
            bool operator==(const document&) const = default;
        };
    }  // namespace canada

    // citm_catalog.json: maps keyed by numeric IDs and small integer records
    namespace citm_catalog
    {
        struct event_t
        {
            std::optional<std::string> description;
            std::int64_t id;
            std::optional<std::string> logo;
            std::string name;
            std::vector<std::int64_t> subTopicIds;
            std::optional<std::string> subjectCode;
            std::optional<std::string> subtitle;
            std::vector<std::int64_t> topicIds;
            bool operator==(const event_t&) const = default;
        };

        struct price_t
        {
            std::int64_t amount;
            std::int64_t audienceSubCategoryId;
            std::int64_t seatCategoryId;
            bool operator==(const price_t&) const = default;
        };

        struct area_t
        {
            std::int64_t areaId;
            std::vector<std::int64_t> blockIds;
            bool operator==(const area_t&) const = default;
        };

        struct seat_category_t
        {
            std::vector<area_t> areas;
            std::int64_t seatCategoryId;
            bool operator==(const seat_category_t&) const = default;
        };

        struct performance_t
        {
            std::int64_t eventId;
            std::int64_t id;
            std::optional<std::string> logo;
            std::optional<std::string> name;
            std::vector<price_t> prices;
            std::vector<seat_category_t> seatCategories;
            std::optional<std::string> seatMapImage;
            std::int64_t start;
            std::string venueCode;
            bool operator==(const performance_t&) const = default;
        };

        struct document
        {
            std::map<std::string, std::string> areaNames;
            std::map<std::string, std::string> audienceSubCategoryNames;
            std::map<std::string, std::string> blockNames;
            std::map<std::string, event_t> events;
            std::vector<performance_t> performances;
            std::map<std::string, std::string> seatCategoryNames;
            std::map<std::string, std::string> subTopicNames;
            std::map<std::string, std::string> subjectNames;
            std::map<std::string, std::string> topicNames;
            std::map<std::string, std::vector<std::int64_t>> topicSubTopics;
            std::map<std::string, std::string> venueNames;
            bool operator==(const document&) const = default;
        };
    }  // namespace citm_catalog

    // twitter.json: a hundred statuses, string-heavy objects with many keys
    namespace twitter
    {
        struct metadata_t
        {
            std::string result_type;
            std::string iso_language_code;
            bool operator==(const metadata_t&) const = default;
        };

        struct url_t
        {
            std::string url;
            std::string expanded_url;
            std::string display_url;
            std::vector<std::int64_t> indices;
            bool operator==(const url_t&) const = default;
        };

        struct urls_t
        {
            std::vector<url_t> urls;
            bool operator==(const urls_t&) const = default;
        };

        struct user_entities_t
        {
            urls_t description;
            std::optional<urls_t> url;
            bool operator==(const user_entities_t&) const = default;
        };

        struct user_t
        {
            std::int64_t id;
            std::string id_str;
            std::string name;
            std::string screen_name;
            std::string location;
            std::string description;
            std::optional<std::string> url;
            user_entities_t entities;
            bool protected_ = false;
            std::int64_t followers_count;
            std::int64_t friends_count;
            std::int64_t listed_count;
            std::string created_at;
            std::int64_t favourites_count;
            std::optional<std::int64_t> utc_offset;
            std::optional<std::string> time_zone;
            bool geo_enabled;
            bool verified;
            std::int64_t statuses_count;
            std::string lang;
            bool contributors_enabled;
            bool is_translator;
            bool is_translation_enabled;
            std::string profile_background_color;
            std::string profile_background_image_url;
            std::string profile_background_image_url_https;
            bool profile_background_tile;
            std::string profile_image_url;
            std::string profile_image_url_https;
            std::optional<std::string> profile_banner_url;
            std::string profile_link_color;
            std::string profile_sidebar_border_color;
            std::string profile_sidebar_fill_color;
            std::string profile_text_color;
            bool profile_use_background_image;
            bool default_profile;
            bool default_profile_image;
            bool following;
            bool follow_request_sent;
            bool notifications;
            bool operator==(const user_t&) const = default;
        };

        struct hashtag_t
        {
            std::string text;
            std::vector<std::int64_t> indices;
            bool operator==(const hashtag_t&) const = default;
        };

        struct user_mention_t
        {
            std::string screen_name;
            std::string name;
            std::int64_t id;
            std::string id_str;
            std::vector<std::int64_t> indices;
            bool operator==(const user_mention_t&) const = default;
        };

        struct size_t_
        {
            std::int64_t w;
            std::int64_t h;
            std::string resize;
            bool operator==(const size_t_&) const = default;
        };

        struct sizes_t
        {
            size_t_ medium;
            size_t_ small;
            size_t_ thumb;
            size_t_ large;
            bool operator==(const sizes_t&) const = default;
        };

        struct media_t
        {
            std::int64_t id;
            std::string id_str;
            std::vector<std::int64_t> indices;
            std::string media_url;
            std::string media_url_https;
            std::string url;
            std::string display_url;
            std::string expanded_url;
            std::string type;
            sizes_t sizes;
            std::optional<std::int64_t> source_status_id;
            std::optional<std::string> source_status_id_str;
            bool operator==(const media_t&) const = default;
        };

        struct entities_t
        {
            std::vector<hashtag_t> hashtags;
            std::vector<std::string> symbols;
            std::vector<url_t> urls;
            std::vector<user_mention_t> user_mentions;
            std::optional<std::vector<media_t>> media;
            bool operator==(const entities_t&) const = default;
        };

// The retweeted status has every member of a status except a retweeted status of its own.
#define BENCH_TYPED_TWITTER_STATUS_HEAD                    \
    metadata_t metadata;                                   \
    std::string created_at;                                \
    std::int64_t id;                                       \
    std::string id_str;                                    \
    std::string text;                                      \
    std::string source;                                    \
    bool truncated;                                        \
    std::optional<std::int64_t> in_reply_to_status_id;     \
    std::optional<std::string> in_reply_to_status_id_str;  \
    std::optional<std::int64_t> in_reply_to_user_id;       \
    std::optional<std::string> in_reply_to_user_id_str;    \
    std::optional<std::string> in_reply_to_screen_name;    \
    user_t user;                                           \
    std::optional<std::string> geo;                        \
    std::optional<std::string> coordinates;                \
    std::optional<std::string> place;                      \
    std::optional<std::string> contributors;
#define BENCH_TYPED_TWITTER_STATUS_TAIL     \
    std::int64_t retweet_count;             \
    std::int64_t favorite_count;            \
    entities_t entities;                    \
    bool favorited;                         \
    bool retweeted;                         \
    std::optional<bool> possibly_sensitive; \
    std::string lang;

        struct retweeted_status_t
        {
            BENCH_TYPED_TWITTER_STATUS_HEAD
            BENCH_TYPED_TWITTER_STATUS_TAIL
            bool operator==(const retweeted_status_t&) const = default;
        };

        struct status_t
        {
            BENCH_TYPED_TWITTER_STATUS_HEAD
            std::optional<retweeted_status_t> retweeted_status;
            BENCH_TYPED_TWITTER_STATUS_TAIL
            bool operator==(const status_t&) const = default;
        };

#undef BENCH_TYPED_TWITTER_STATUS_HEAD
#undef BENCH_TYPED_TWITTER_STATUS_TAIL

        struct search_metadata_t
        {
            double completed_in;
            std::int64_t max_id;
            std::string max_id_str;
            std::string next_results;
            std::string query;
            std::string refresh_url;
            std::int64_t count;
            std::int64_t since_id;
            std::string since_id_str;
            bool operator==(const search_metadata_t&) const = default;
        };

        struct document
        {
            std::vector<status_t> statuses;
            search_metadata_t search_metadata;
            bool operator==(const document&) const = default;
        };
    }  // namespace twitter
}  // namespace bench_typed
// NOLINTEND
