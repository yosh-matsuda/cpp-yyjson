[Documentation](README.md) › Upgrading

# Upgrading from 2.1 to 2.2

Version 2.2 adds constraints, read policies and JSON Schema. This page lists the changes in behavior that can affect a program that compiled with 2.1. Each item tells what changed, which programs it affects, and what to do.

## The text of `bad_cast::what()`

**What changed.** The text of `yyjson::bad_cast::what()` now starts with the text of `message()`. The path and the position follow, and a note for the developer ends it:

```text
2.1: int is not constructible from JSON string at /x/a/k (line 1, column 37, pos 36)
2.2: expected an integer, got a string at /x/a/k (line 1, column 37, pos 36); reading int
```

A `bad_cast` that your caster throws keeps its text at the start of `what()`, and its `message()` is `the value is not valid`.

**Who is affected.** A program or a test that compares the text of `what()`.

**What to do.** Compare `kind()`, `path()` and `position()`, or compare `message()`. See [Conversion errors](validation.md#conversion-errors).

## Exceptions in an untagged variant

**What changed.** If an alternative of an untagged `std::variant` throws `yyjson::bad_cast`, the variant tries the next alternative, as in 2.1. Another exception, such as a `std::logic_error` that your caster throws, now leaves the read. In 2.1, the variant tried the next alternative. When no alternative reads the value, the read can call a caster again to explain a failure it did not record.

**Who is affected.** A program with a caster for an alternative of an untagged variant. The caster throws an exception other than `bad_cast` for a value that it does not accept.

**What to do.** Make the caster throw `yyjson::bad_cast` for a value that it does not accept. See [Untagged variants](conversions.md#untagged-variants).

## A field caster in a skipped struct alternative

**What changed.** If a member has a kind its field never reads, a DOM cast of an untagged variant can skip the struct. For example, a string cannot fill an `int` field. In 2.1, a caster for an earlier member could run and throw `std::logic_error` before the cast reached that field. The variant caught the exception and tried the next alternative. In 2.2, the cast can select the same alternative without calling the caster.

**Who is affected.** A program that relies on a field caster's side effects while a variant tries its alternatives.

**What to do.** Act on the selected alternative after the cast. See [Untagged variants](conversions.md#untagged-variants).

## Reading an object as a single pair

**What changed.** Both versions reject `read<std::pair<std::string, int>>(R"({"a":1})")`. In 2.1, `what()` said the pair `is not constructible from JSON`. In 2.2, `message()` is `expected an array, got an object`.

**Who is affected.** A program or test that compares the diagnostic for a pair.

**What to do.** To read one pair, supply an array such as `["a",1]`. To identify a type mismatch, compare `kind()` with `yyjson::error_kind::type`.

## Writing a tag beside an unnamed `std::monostate`

**What changed.** A written adjacent tag field beside an unnamed `std::monostate` now makes `write` throw `std::invalid_argument`, even for `""`. For a string alternative named `s`, 2.1 wrote `{"kind":"s","d":null}`. Reading it back failed because the string could not read `null`.

**Who is affected.** A struct stores the tag in a field that the write does not omit. Reading `{"d":null}` can leave an empty tag that `write` rejects.

**What to do.** To omit the empty tag, declare `yyjson::field("kind", yyjson::default_, yyjson::min_length(1))`. Alternatively, name `std::monostate` in `ids` and set the tag field to that name. See [A field that holds the tag](conversions.md#a-field-that-holds-the-tag).

## An enumerator value without a name

**What changed.** For a value of an enumeration that has no name, such as `static_cast<Color>(7)`, `write` now throws `yyjson::write_error` instead of `yyjson::bad_cast`. `yyjson::value(t)` and the other constructors of the DOM do the same.

**Who is affected.** A program that catches `bad_cast` around a write of such a value.

**What to do.** Catch `yyjson::write_error`. To write such a value, give the enumeration `yyjson::enum_format::integer` or `yyjson::enum_format::name_or_integer`. See [Formats and renamed enumerators](conversions.md#formats-and-renamed-enumerators).

## `null` in a new value of `merge_into`

**What changed.** `merge_into` removes the members whose value is `null` from each object of a patch that gives a new value. A new value has no object before the merge. Examples are a new member of a map, an empty `std::optional` and a new alternative of a tagged variant. Thus:

*   A new member of a `std::map<std::string, yyjson::value>` gets `{"a":1}` from `{"k":{"z":null,"a":1}}`. 2.1 kept `"z":null`. The same holds for a new value that is a range of pairs, such as `std::vector<std::pair<std::string, std::optional<int>>>`.
*   A switch of a tagged variant reads `{"type":"circle","r":null}` as a circle with the default `r`. 2.1 threw `bad_cast`.

**Who is affected.** A program that merges a patch with `null` into a value that the patch makes new, and that keeps the `null` or expects the exception.

**What to do.** Do not expect the `null` members in the new value, and do not expect `bad_cast` for such a switch. See [Apply a partial update](guide.md#apply-a-partial-update).

## `null` for a variant with an adjacent tag

**What changed.** Consider a field `body` with `yyjson::adjacent_tag("t", ...)`. `merge_into` of `{"body":null}` now throws `bad_cast`, because the tag `t` stays and names an alternative without a value. 2.1 cleared the field.

**Who is affected.** A program that clears such a field with `{"body":null}`.

**What to do.** Give `{"body":null,"t":null}` to remove the value and its tag. See [Tagged variants with `read_into` and `merge_into`](conversions.md#tagged-variants-with-read_into-and-merge_into).

## C++26 annotations on a struct

**What changed.** With C++26 static reflection, the library now reads the annotations on a struct. A setting of a member on a struct, such as `struct [[=yyjson::skip]] S`, fails to compile. 2.1 ignored it.

**Who is affected.** A program that compiles with C++26 static reflection and puts a setting of a member on a struct.

**What to do.** Remove the setting from the struct, or move it to the member that it is for. See [C++26 annotations](conversions.md#c26-annotations).
