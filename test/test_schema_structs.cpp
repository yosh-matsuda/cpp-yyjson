#include "test_schema_common.hpp"

// NOLINTBEGIN

TEST(Schema, Structs)
{
    check<Point>({R"({"x":1,"y":2})",
                  R"({"x":1})",
                  "{}",
                  R"({"x":1,"y":2,"z":3})",
                  R"({"x":"1"})",
                  "[1,2]",
                  {R"({"x":1,"y":2,"x":3})", not_expressible}});
    check<Node>({R"({"name":"a","children":[{"name":"b","children":[]}]})", R"({"children":[{"name":1}]})"});
    check<Tree>({R"({"value":1,"kids":[{"value":2,"kids":[]}]})", R"({"kids":[{"value":"x"}]})"});
    check<Renamed>({R"({"A":1,"b":2})", R"({"a":1})", R"({"A":"x"})", R"({"cache":"x"})"});
    check<Optional>({R"({"a":1,"o":2,"p":{"x":1,"y":2},"n":3,"u":4})",
                     R"({"n":null,"u":null})",
                     {R"({"a":1,"n":null,"u":null,"o":null})", lenient_null},
                     {R"({"a":1,"n":null,"u":null,"p":null})", lenient_null},
                     R"({"o":"x"})"});
    check<FieldRequired>({R"({"a":1,"c":2})", R"({"a":1})", R"({"c":1})", R"({"a":1,"c":null})"});
    check<StructRequired>(
        {R"({"b":"x"})", "{}", R"({"a":1})", R"({"b":null})", {R"({"a":1,"b":"x","c":null})", lenient_null}});
    check<Closed>({R"({"a":1,"b":"x"})", "{}", R"({"a":1,"c":2})"});
    check<Defaults>({"{}", R"({"a":5})", R"({"a":"x"})"});
    check<StructDefault>({"{}", R"({"a":5})"});
    check<Visited>({R"({"a":1,"b":"x"})", R"({"b":1})"});
    check<Fields>({R"({"flag":true,"letter":65,"byte":255,"ratio":0.5,"color":"red","pair":[1,2],"names":["a","b"],)"
                   R"("bits":[true],"by_color":{"red":1},"tuple":[1,"a"],"shared":null})",
                   R"({"pair":[1]})", R"({"by_color":{"pink":1}})", R"({"shared":{"x":1,"y":2}})"});

    const auto point = definition<Point>(false);
    EXPECT_EQ(point.to_string(),
              R"({"properties":{"x":{"default":0,"maximum":2147483647,"minimum":-2147483648,"type":"integer"},)"
              R"("y":{"default":0,"maximum":2147483647,"minimum":-2147483648,"type":"integer"}},"required":["x","y"],)"
              R"("type":"object"})");
    EXPECT_EQ(schema<Point>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.Point":{"type":)"
              R"("object","properties":{"x":{"type":"integer","minimum":-2147483648,"maximum":2147483647,)"
              R"("default":0},"y":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},)"
              R"("required":["x","y"]}},"$ref":"#/$defs/schema_test.Point"})");

    // the order of the properties is that of the fields, and a skipped field has none
    EXPECT_EQ(property_keys<Renamed>(), (std::vector<std::string>{"A", "b"}));
    EXPECT_EQ(required_keys(definition<Renamed>(true)), (std::set<std::string>{"A", "b"}));
    EXPECT_EQ(property_keys<Visited>(), (std::vector<std::string>{"b", "a"}));

    // a recursive type refers to itself
    EXPECT_EQ(definition<Node>(false)["properties"]["children"]["items"]["$ref"].as<std::string>(),
              "#/$defs/schema_test.Node");
    EXPECT_EQ(definition<Tree>(true)["properties"]["kids"]["items"]["$ref"].as<std::string>(),
              "#/$defs/schema_test.Tree");

    // a std::optional field has the schema of its value, and a field of `nullable` takes null
    const auto optional = definition<Optional>(false);
    EXPECT_EQ(optional["properties"]["o"]["type"].as<std::string>(), "integer");
    EXPECT_EQ(optional["properties"]["p"]["$ref"].as<std::string>(), "#/$defs/schema_test.Point");
    EXPECT_EQ(optional["properties"]["n"]["type"].to_string(), R"(["integer","null"])");
    EXPECT_EQ(optional["properties"]["u"]["type"].to_string(), R"(["integer","null"])");

    // only a type that declares it is closed
    EXPECT_FALSE(definition<Closed>(false)["additionalProperties"].as<bool>());
    EXPECT_FALSE(definition<Closed>(true)["additionalProperties"].as<bool>());
    EXPECT_FALSE(definition<Point>(true).contains("additionalProperties"));
}

TEST(Schema, Required)
{
    EXPECT_EQ(required_keys(definition<Point>(false)), (std::set<std::string>{"x", "y"}));
    EXPECT_EQ(required_keys(definition<FieldRequired>(false)), (std::set<std::string>{"a", "b", "c"}));
    EXPECT_EQ(required_keys(definition<StructRequired>(false)), (std::set<std::string>{"a", "b"}));
    EXPECT_EQ(required_keys(definition<Optional>(true)), (std::set<std::string>{"a", "n", "u"}));
    EXPECT_EQ(required_keys(definition<FieldRequired>(true)), (std::set<std::string>{"a", "b", "c"}));
    EXPECT_EQ(required_keys(definition<StructRequired>(true)), (std::set<std::string>{"a", "b"}));
    EXPECT_EQ(required_keys(definition<Defaults>(true)), (std::set<std::string>{"b", "c"}));
    EXPECT_EQ(required_keys(definition<StructDefault>(true)), std::set<std::string>{});
    EXPECT_FALSE(definition<StructDefault>(true).contains("required"));

    check_write_required<Point>({Point{}, Point{1, 2}});
    auto optional = std::vector<Optional>(2);
    optional[1] = Optional{1, 2, Point{}, 3, std::make_unique<int>(4)};
    check_write_required(optional);
    check_write_required<FieldRequired>({FieldRequired{}, FieldRequired{1, 2, 3}});
    check_write_required<StructRequired>({StructRequired{}, StructRequired{1, "x", 2}});
    check_write_required<Defaults>({Defaults{}, Defaults{5, 6, "x"}});
    check_write_required<StructDefault>({StructDefault{}, StructDefault{5, 6}});
}

TEST(Schema, Names)
{
    // a name that a type found before has gets the first free suffix
    const auto holder = to_jsoncons(yyjson::json_schema<Holder>());
    const auto& defs = holder["$defs"];
    EXPECT_EQ(defs.size(), 3);
    EXPECT_TRUE(defs.contains("schema_test.Holder"));
    EXPECT_EQ(defs["schema_test.Box_1_"]["properties"]["v"]["type"].as<std::string>(), "integer");
    EXPECT_EQ(defs["schema_test.Box_1__2"]["properties"]["s"]["type"].as<std::string>(), "string");
    const auto& properties = defs["schema_test.Holder"]["properties"];
    EXPECT_EQ(properties["a"]["$ref"].as<std::string>(), "#/$defs/schema_test.Box_1_");
    EXPECT_EQ(properties["b"]["$ref"].as<std::string>(), "#/$defs/schema_test.Box_1__2");
    EXPECT_EQ(properties["c"]["$ref"].as<std::string>(), "#/$defs/schema_test.Box_1_");
    check<Holder>({R"({"a":{"v":1},"b":{"s":"x"},"c":{"v":2}})", R"({"b":{"s":1}})", R"({"a":{"s":"x"}})"});
}

TEST(Schema, SplitNames)
{
    // a type has a name for each direction while its two schemas differ, and so has a type that refers to it
    auto b = yyjson::schema_detail::builder("#/$defs/");
    b.add<Split>(false);
    b.add<Split>(true);
    auto doc = yyjson::value(yyjson::object());
    auto root = *doc.as_object();
    b.emit_definitions(root.emplace("$defs", yyjson::empty_object));
    b.emit_root<Split>(root.emplace("read", yyjson::empty_object), false);
    b.emit_root<Split>(root.emplace("write", yyjson::empty_object), true);
    const auto s = json::parse(std::string(doc.write()));
    auto names = std::set<std::string>();
    for (const auto& m : s["$defs"].object_range()) names.insert(m.key());
    EXPECT_EQ(names,
              (std::set<std::string>{"schema_test.Emitted-Input", "schema_test.Emitted-Output", "schema_test.Maybe",
                                     "schema_test.Split-Input", "schema_test.Split-Output"}));
    EXPECT_EQ(s["read"]["$ref"].as<std::string>(), "#/$defs/schema_test.Split-Input");
    EXPECT_EQ(s["write"]["$ref"].as<std::string>(), "#/$defs/schema_test.Split-Output");
    const auto& output = s["$defs"]["schema_test.Split-Output"]["properties"];
    EXPECT_EQ(output["f"]["$ref"].as<std::string>(), "#/$defs/schema_test.Emitted-Output");
    EXPECT_EQ(output["p"]["$ref"].as<std::string>(), "#/$defs/schema_test.Maybe");
}

TEST(Schema, Options)
{
    EXPECT_EQ(schema<int>(yyjson::schema_options{.id = "https://example.com/int.json"}),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$id":"https://example.com/int.json",)"
              R"("type":"integer","minimum":-2147483648,"maximum":2147483647})");
    // the references to the named types take the prefix, while their schemas stay in `$defs`
    EXPECT_EQ(schema<std::vector<Hue>>(yyjson::write_direction,
                                       yyjson::schema_options{.ref_prefix = "#/components/schemas/"}),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.Hue":{"type":)"
              R"("string","enum":["red","green","blue"]}},"type":"array","items":{"$ref":"#/components/schemas/)"
              R"(schema_test.Hue"}})");
    const auto id = std::string("urn:example:point");
    const auto point = to_jsoncons(yyjson::json_schema<Point>(yyjson::schema_options{.id = id}));
    EXPECT_EQ(point["$id"].as<std::string>(), id);
    EXPECT_TRUE(compile(point).is_valid(json::parse(R"({"x":1,"y":2})")));
    EXPECT_FALSE(compile(point).is_valid(json::parse(R"({"x":1})")));
    EXPECT_FALSE(compile(point).is_valid(json::parse(R"({"x":"1"})")));
}

TEST(Schema, Defaults)
{
    const auto read = definition<WithDefaults>(false);
    const auto& p = read["properties"];
    EXPECT_EQ(p["a"]["default"].as<int>(), 1);
    EXPECT_EQ(p["in"]["default"], json::parse(R"({"x":3,"v":[1,2]})"));
    EXPECT_EQ(p["from"]["default"].as<int>(), 7);
    EXPECT_EQ(p["named"]["default"].as<std::string>(), "anon");
    EXPECT_TRUE(p["n"]["default"].is_null());
    EXPECT_TRUE(p["u"]["default"].is_null());
    EXPECT_FALSE(p["none"].contains("default"));
    EXPECT_EQ(p["held"]["default"].as<int>(), 5);
    EXPECT_FALSE(p["nan"].contains("default"));
    EXPECT_FALSE(p["req"].contains("default"));
    EXPECT_EQ(p["m"]["default"], json::parse(R"({"k":1})"));
    EXPECT_EQ(p["pair"]["default"], json::parse("[7,8]"));
    // every schema has the same defaults
    for (const auto& other :
         {definition<WithDefaults>(true), definition_with<WithDefaults>(yyjson::strict, yyjson::reject_unknown_keys),
          definition_with<WithDefaults>(yyjson::write_direction, yyjson::emit_defaults)})
        for (const auto& m : p.object_range())
        {
            const auto& o = other["properties"][m.key()];
            EXPECT_EQ(o.contains("default"), m.value().contains("default")) << m.key();
            if (o.contains("default"))
            {
                EXPECT_EQ(o["default"], m.value()["default"]) << m.key();
            }
        }
    check<WithDefaults>(
        {R"({"req":1,"nan":1.5})",
         R"({"nan":1.5})",
         R"({"req":1,"nan":1.5,"in":{"x":"a"}})",
         R"({"req":1,"nan":1.5,"n":null,"held":2})",
         {R"({"a":1,"in":{"x":1,"v":[]},"n":null,"u":null,"nan":1.5,"req":1,"m":{},"pair":[1,2],"held":null})",
          lenient_null}});

    // a generation calls the function of `yyjson::default_from` once
    const auto before = calls;
    EXPECT_EQ(definition<Called>(false)["properties"]["a"]["default"].as<int>(), 7);
    EXPECT_EQ(calls, before + 1);

    // a struct without `S()` has the defaults of `yyjson::default_from` only
    const auto fixed = definition_with<Fixed>(yyjson::write_direction);
    EXPECT_FALSE(fixed["properties"]["a"].contains("default"));
    EXPECT_EQ(fixed["properties"]["b"]["default"].as<int>(), 7);

    // a value in `S()` that violates the keywords of its field is no default
    auto names = yyjson::schema_detail::registry();
    EXPECT_FALSE((yyjson::schema_detail::field_default<Sentinel, 0>(names)));
    EXPECT_EQ((yyjson::schema_detail::field_default<Sentinel, 1>(names)), "5");
    EXPECT_FALSE((yyjson::schema_detail::field_default<Sentinel, 2>(names)));
    EXPECT_FALSE((yyjson::schema_detail::field_default<DefaultSentinel, 0>(names)));
    EXPECT_EQ((yyjson::schema_detail::field_default<DefaultSentinel, 1>(names)), "5");
    EXPECT_FALSE((yyjson::schema_detail::field_default<DefaultSentinel, 2>(names)));
    EXPECT_FALSE((yyjson::schema_detail::field_default<DefaultSentinel, 3>(names)));

    const auto narrowed = definition<Narrowed>(false)["properties"];
    EXPECT_EQ(narrowed["wide"]["default"], json::parse(R"({"100":1})"));
    EXPECT_EQ(narrowed["fits"]["default"], json::parse(R"({"-1":2,"99":1})"));
    EXPECT_EQ(narrowed["nested"]["default"], json::parse(R"([{"-100":1}])"));
    EXPECT_FALSE(narrowed["mixed"].contains("default"));
    EXPECT_EQ(narrowed["same"]["default"], json::parse(R"({"a":1,"b":2})"));
    EXPECT_EQ(narrowed["checked"]["default"], json::parse(R"({"p":{"100":1}})"));
    EXPECT_EQ(narrowed["checks"]["default"], json::parse(R"({"p":{"99":1}})"));
    check<Narrowed>({"{}"});
}

TEST(Schema, WriteRequired)
{
    check_write_required<Point>({Point{}, Point{1, 2}}, yyjson::emit_defaults);
    auto optional = std::vector<Optional>(2);
    optional[1] = Optional{1, 2, Point{}, 3, std::make_unique<int>(4)};
    check_write_required(optional, yyjson::emit_defaults);
    check_write_required<Defaults>({Defaults{}, Defaults{5, 6, "x"}}, yyjson::emit_defaults);
    check_write_required<StructDefault>({StructDefault{}, StructDefault{5, 6}}, yyjson::emit_defaults);
    EXPECT_EQ(required_keys(definition_with<StructDefault>(yyjson::write_direction, yyjson::emit_defaults)),
              (std::set<std::string>{"a", "b"}));
    const auto init = std::vector<OptionalInit>{OptionalInit{1, std::nullopt}, OptionalInit{}};
    check_write_required(init);
    check_write_required(init, yyjson::emit_defaults);
    EXPECT_EQ(definition<OptionalInit>(false)["properties"]["o"]["default"].as<int>(), 1);

    auto defaults = std::vector<WithDefaults>(2);
    defaults[0].from = 7;
    defaults[0].held.reset();
    defaults[0].nan = 0;
    defaults[1].from = 8;
    defaults[1].named = "x";
    defaults[1].none = 1;
    defaults[1].nan = 0;
    check_write_required(defaults);
    check_write_required(defaults, yyjson::emit_defaults);
    EXPECT_EQ(required_keys(definition<WithDefaults>(true)),
              (std::set<std::string>{"a", "in", "n", "u", "nan", "req", "m", "pair"}));

    // a struct that counts its members writes every default, which `json_schema` does not emit yet
    namespace sd = yyjson::schema_detail;
    EXPECT_TRUE((sd::write_requires<Counted, 0>(false)));
    EXPECT_TRUE((sd::write_requires<Counted, 1>(false)));
    EXPECT_FALSE((sd::write_requires<StructDefault, 0>(false)));
    EXPECT_TRUE((sd::write_requires<StructDefault, 0>(true)));
    // no write gives a sentinel, even under `yyjson::emit_defaults`
    for (const bool emit : {false, true})
    {
        EXPECT_FALSE((sd::write_requires<DefaultSentinel, 0>(emit)));
        EXPECT_TRUE((sd::write_requires<DefaultSentinel, 1>(emit)));
        EXPECT_FALSE((sd::write_requires<DefaultSentinel, 2>(emit)));
        EXPECT_FALSE((sd::write_requires<DefaultSentinel, 3>(emit)));
    }
    EXPECT_EQ(std::string(yyjson::write(DefaultSentinel{-1, 5, -1, -5}, yyjson::emit_defaults)), R"({"b":5})");
    const auto counted = yyjson::read(yyjson::write(Counted{}));
    EXPECT_EQ(counted.as_object()->size(), 2);
}

TEST(Schema, Policies)
{
    // the tags of the call apply to every struct
    EXPECT_EQ(required_keys(definition_with<Point>(yyjson::reject_missing_keys)), (std::set<std::string>{"x", "y"}));
    EXPECT_EQ(required_keys(definition_with<Optional>(yyjson::strict)), (std::set<std::string>{"a", "n", "u"}));
    EXPECT_EQ(required_keys(definition_with<Defaults>(yyjson::strict)), (std::set<std::string>{"b", "c"}));
    EXPECT_FALSE(definition_with<StructDefault>(yyjson::strict).contains("required"));
    EXPECT_EQ(required_keys(definition_with<FieldRequired>(yyjson::reject_unknown_keys)),
              (std::set<std::string>{"a", "b", "c"}));
    EXPECT_FALSE(definition_with<Point>(yyjson::reject_unknown_keys)["additionalProperties"].as<bool>());
    EXPECT_FALSE(definition_with<Point>(yyjson::strict).contains("additionalProperties"));
    EXPECT_EQ(schema<Optional>(yyjson::reject_null_as_missing, yyjson::reject_duplicates), schema<Optional>());
    const auto holder = to_jsoncons(yyjson::json_schema<Holder>(yyjson::strict, yyjson::reject_unknown_keys));
    for (const auto& m : holder["$defs"].object_range())
    {
        EXPECT_FALSE(m.value()["additionalProperties"].as<bool>()) << m.key();
        EXPECT_FALSE(required_keys(m.value()).empty()) << m.key();
    }

    // the policies of a type apply to its own schema
    const auto outer = to_jsoncons(yyjson::json_schema<PolicyOuter>());
    const auto& defs = outer["$defs"];
    EXPECT_EQ(required_keys(defs["schema_test.PolicyOuter"]), (std::set<std::string>{"z", "s", "m", "u", "p"}));
    EXPECT_FALSE(defs["schema_test.PolicyOuter"].contains("additionalProperties"));
    EXPECT_EQ(required_keys(defs["schema_test.StrictInner"]), (std::set<std::string>{"a", "c"}));
    EXPECT_FALSE(defs["schema_test.StrictInner"].contains("additionalProperties"));
    EXPECT_EQ(required_keys(defs["schema_test.MissingInner"]), (std::set<std::string>{"a"}));
    EXPECT_EQ(required_keys(defs["schema_test.UnknownInner"]), (std::set<std::string>{"a"}));
    EXPECT_FALSE(defs["schema_test.UnknownInner"]["additionalProperties"].as<bool>());
    EXPECT_EQ(defs["schema_test.Point"], definition<Point>(false));
    EXPECT_EQ(defs["schema_test.StrictInner"], definition<StrictInner>(false));
    // a default has every member, as the write with `yyjson::emit_defaults` gives it, so every schema takes it
    EXPECT_EQ(defs["schema_test.PolicyOuter"]["properties"]["m"]["default"], json::parse(R"({"a":0,"c":2})"));
    // yyjson::reject_unknown_keys of the call closes every struct below the root, beside the policies of each type
    const auto closed = to_jsoncons(yyjson::json_schema<PolicyOuter>(yyjson::reject_unknown_keys));
    for (const auto& m : closed["$defs"].object_range())
        EXPECT_FALSE(m.value()["additionalProperties"].as<bool>()) << m.key();
    EXPECT_EQ(required_keys(closed["$defs"]["schema_test.StrictInner"]), (std::set<std::string>{"a", "c"}));
    // a read policy has no effect on the write schema
    EXPECT_FALSE(definition<UnknownInner>(true).contains("additionalProperties"));
    EXPECT_EQ(required_keys(definition<StrictInner>(true)), (std::set<std::string>{"a", "c"}));
    check<PolicyOuter>(
        {R"({"z":1,"s":{"a":1,"c":2},"m":{"a":1},"u":{"a":1},"p":{}})",
         R"({"s":{"a":1}})",
         R"({"s":{"a":1,"c":2,"q":3}})",
         R"({"s":{"a":1,"c":2,"b":null}})",
         R"({"m":{}})",
         R"({"m":{"a":1,"q":3}})",
         R"({"u":{"a":1,"q":2}})",
         R"({"u":{}})",
         R"({"p":{"x":1,"q":2}})",
         {R"({"z":1,"s":{"a":1,"c":2},"m":{"a":1},"u":{"a":1,"a":2},"p":{"x":1,"y":2}})", not_expressible}});
}

// NOLINTEND
