// NOLINTBEGIN
#pragma once

// Models of the benchmark datasets for reading JSON into and writing it from C++ types.
//
// The same aggregates are read and written by cpp-yyjson, through its field-name
// reflection, and by Glaze, through its own, so both libraries fill the same objects.
// Members are declared in the order their keys appear in the documents, which is also
// the order both libraries write them in.  Every key of the documents has a member.  A
// key that is not a valid member name, such as `@context` or the C++ keyword `public`,
// belongs to a member of another name, which `yyjson::field_rename` maps to the key; the
// Glaze executable gives Glaze the same mapping through `glz::meta<T>::rename_key`.
// Nothing is skipped, so a library that is fast at skipping values gains little here.
//
// A key whose value is always null in the document is a `std::optional`, as is a key
// that only some of the objects have.  Both libraries leave out an empty optional when
// writing, so they write the same keys.

#include <cpp_yyjson.hpp>
#if defined(CPPYYJSON_GLAZE_BENCHMARKS)
#include <glaze/glaze.hpp>
#endif
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
            bool protected_;
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

    // poet.json: nine thousand records of three strings, most of them long Chinese texts
    namespace poet
    {
        struct poet_t
        {
            std::string desc;
            std::string name;
            std::string id;
            bool operator==(const poet_t&) const = default;
        };

        using document = std::vector<poet_t>;
    }  // namespace poet

    // github_events.json: thirty events of seven kinds, whose payloads share one struct in
    // which a key that only some of the kinds have is a `std::optional`
    namespace github_events
    {
        struct actor_t
        {
            std::string gravatar_id;
            std::string login;
            std::string avatar_url;
            std::string url;
            std::int64_t id;
            bool operator==(const actor_t&) const = default;
        };

        struct repo_t
        {
            std::string url;
            std::int64_t id;
            std::string name;
            bool operator==(const repo_t&) const = default;
        };

        struct author_t
        {
            std::string email;
            std::string name;
            bool operator==(const author_t&) const = default;
        };

        struct commit_t
        {
            std::string url;
            std::string message;
            bool distinct;
            std::string sha;
            author_t author;
            bool operator==(const commit_t&) const = default;
        };

        struct user_t
        {
            std::string url;
            std::string gists_url;
            std::string gravatar_id;
            std::string type;
            std::string avatar_url;
            std::string subscriptions_url;
            std::string organizations_url;
            std::string received_events_url;
            std::string repos_url;
            std::string login;
            std::int64_t id;
            std::string starred_url;
            std::string events_url;
            std::string followers_url;
            std::string following_url;
            bool operator==(const user_t&) const = default;
        };

        struct forkee_t
        {
            std::string description;
            bool fork;
            std::string url;
            std::string language;
            std::string stargazers_url;
            std::string clone_url;
            std::string tags_url;
            std::string full_name;
            std::string merges_url;
            std::int64_t forks;
            bool private_;
            std::string git_refs_url;
            std::string archive_url;
            std::string collaborators_url;
            user_t owner;
            std::string languages_url;
            std::string trees_url;
            std::string labels_url;
            std::string html_url;
            std::string pushed_at;
            std::string created_at;
            bool has_issues;
            std::string forks_url;
            std::string branches_url;
            std::string commits_url;
            std::string notifications_url;
            std::int64_t open_issues;
            std::string contents_url;
            std::string blobs_url;
            std::string issues_url;
            std::string compare_url;
            std::string issue_events_url;
            std::string name;
            std::string updated_at;
            std::string statuses_url;
            std::int64_t forks_count;
            std::string assignees_url;
            std::string ssh_url;
            bool public_;
            bool has_wiki;
            std::string subscribers_url;
            std::optional<std::string> mirror_url;
            std::int64_t watchers_count;
            std::int64_t id;
            bool has_downloads;
            std::string git_commits_url;
            std::string downloads_url;
            std::string pulls_url;
            std::optional<std::string> homepage;
            std::string issue_comment_url;
            std::string hooks_url;
            std::string subscription_url;
            std::string milestones_url;
            std::string svn_url;
            std::string events_url;
            std::string git_tags_url;
            std::string teams_url;
            std::string comments_url;
            std::int64_t open_issues_count;
            std::string keys_url;
            std::string git_url;
            std::string contributors_url;
            std::int64_t size;
            std::int64_t watchers;
            bool operator==(const forkee_t&) const = default;
        };

        struct pull_request_t
        {
            std::optional<std::string> html_url;
            std::optional<std::string> patch_url;
            std::optional<std::string> diff_url;
            bool operator==(const pull_request_t&) const = default;
        };

        struct issue_t
        {
            user_t user;
            std::string url;
            // empty in every issue of the document
            std::vector<std::string> labels;
            std::string html_url;
            std::string labels_url;
            pull_request_t pull_request;
            std::string created_at;
            std::optional<std::string> closed_at;
            std::optional<std::string> milestone;
            std::string title;
            std::string body;
            std::string updated_at;
            std::int64_t number;
            std::string state;
            std::optional<user_t> assignee;
            std::int64_t id;
            std::string events_url;
            std::string comments_url;
            std::int64_t comments;
            bool operator==(const issue_t&) const = default;
        };

        struct comment_t
        {
            user_t user;
            std::string url;
            std::string issue_url;
            std::string created_at;
            std::string body;
            std::string updated_at;
            std::int64_t id;
            bool operator==(const comment_t&) const = default;
        };

        struct page_t
        {
            std::string page_name;
            std::string html_url;
            std::string title;
            std::string sha;
            std::optional<std::string> summary;
            std::string action;
            bool operator==(const page_t&) const = default;
        };

        struct payload_t
        {
            std::optional<std::vector<commit_t>> commits;
            std::optional<std::int64_t> distinct_size;
            std::optional<std::string> ref;
            std::optional<std::int64_t> push_id;
            std::optional<std::string> head;
            std::optional<std::string> before;
            std::optional<std::int64_t> size;
            std::optional<std::string> description;
            std::optional<std::string> master_branch;
            std::optional<std::string> ref_type;
            std::optional<forkee_t> forkee;
            std::optional<std::string> action;
            std::optional<issue_t> issue;
            std::optional<comment_t> comment;
            std::optional<std::vector<page_t>> pages;
            bool operator==(const payload_t&) const = default;
        };

        struct event_t
        {
            std::string type;
            std::string created_at;
            actor_t actor;
            repo_t repo;
            bool public_;
            payload_t payload;
            std::string id;
            std::optional<actor_t> org;
            bool operator==(const event_t&) const = default;
        };

        using document = std::vector<event_t>;
    }  // namespace github_events

    // gsoc-2018.json: twelve hundred schema.org records keyed by their index, whose keys
    // include `@context` and `@type`
    namespace gsoc_2018
    {
        struct sponsor_t
        {
            std::string type;
            std::string name;
            std::string disambiguatingDescription;
            std::string description;
            std::string url;
            std::string logo;
            bool operator==(const sponsor_t&) const = default;
        };

        struct author_t
        {
            std::string type;
            std::string name;
            bool operator==(const author_t&) const = default;
        };

        struct project_t
        {
            std::string context;
            std::string type;
            std::string name;
            std::string description;
            sponsor_t sponsor;
            author_t author;
            bool operator==(const project_t&) const = default;
        };

        using document = std::map<std::string, project_t>;
    }  // namespace gsoc_2018
}  // namespace bench_typed

template <>
inline constexpr auto yyjson::field_rename<bench_typed::twitter::user_t> = {yyjson::rename("protected_", "protected")};
template <>
inline constexpr auto yyjson::field_rename<bench_typed::github_events::forkee_t> = {
    yyjson::rename("private_", "private"),
    yyjson::rename("public_", "public")};
template <>
inline constexpr auto yyjson::field_rename<bench_typed::github_events::event_t> = {yyjson::rename("public_", "public")};
template <>
inline constexpr auto yyjson::field_rename<bench_typed::gsoc_2018::sponsor_t> = {yyjson::rename("type", "@type")};
template <>
inline constexpr auto yyjson::field_rename<bench_typed::gsoc_2018::author_t> = {yyjson::rename("type", "@type")};
template <>
inline constexpr auto yyjson::field_rename<bench_typed::gsoc_2018::project_t> = {
    yyjson::rename("context", "@context"),
    yyjson::rename("type", "@type")};

#if defined(CPPYYJSON_GLAZE_BENCHMARKS)
template <typename T>
requires (yyjson::detail::has_field_rename<T>)
struct glz::meta<T>
{
    static constexpr std::string_view rename_key(const std::string_view member)
    {
        for (const auto& r : yyjson::field_rename<T>)
            if (r.member() == member) return r.key();
        return member;
    }
};
#endif
// NOLINTEND
