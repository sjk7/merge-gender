// This is an independent project of an individual developer. Dear PVS-Studio,
// please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// http://www.viva64.com
#ifdef _MSC_VER
#pragma warning(disable : 4068)
#endif

#pragma clang diagnostic push
#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-identifier-length"
#pragma ide diagnostic ignored "cppcoreguidelines-pro-bounds-pointer-arithmetic"
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-c-arrays"
#pragma ide diagnostic ignored                                                 \
    "cppcoreguidelines-pro-bounds-array-to-pointer-decay"
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-vararg"
#pragma ide diagnostic ignored                                                 \
    "cppcoreguidelines-avoid-non-const-global-variables"
#pragma ide diagnostic ignored "cert-err58-cpp"

// ReSharper disable CppUseRangeAlgorithm
// ReSharper disable CppUseStructuredBinding
#include "../../utils/gender.hpp"
#include "../../utils/my_timing.hpp"

#include <iostream>

using namespace std;
std::string g_source_file;
std::string g_dest_file{"ArtistsGENDER.txt"};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-function-cognitive-complexity"
int parseArgs(const int argc, char** argv) {

    using namespace my::utils;
    using namespace my::utils::strings;
    const auto nvps = parse_args(argc, argv);
    MYASSERT(!nvps.empty(), "No command arguments found")
    const auto found = contains(nvps, "DEFAULT");
    if (found == nvps.end() && nvps.empty()) {
        MYASSERT(false, "args does not even have a default argument")
    }
    const auto& val = *found;

    const std::string serr
        = concat("Destination file path value ", val.value, " is too short");
    MYASSERT(val.value.size() > 4, serr.c_str())

    MYASSERT(my::utils::file_exists(val.value),
        concat("Source file: ", val.value, " is not found on disk").c_str())

    MYASSERT(my::utils::file_exists(g_dest_file),
        concat("Destination file to merge into: ", g_dest_file,
            " is not found on disk")
            .c_str())

    g_source_file = nvps.at(0).value;

    return 0;
}
#pragma clang diagnostic pop

using gendermap_t
    = my::utils::strings::case_insensitive_map<std::string_view, gender_t>;
//= std::map<std::string_view, gender_t>;
std::string g_merge_from_data;
std::string g_merge_into_data;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-function-cognitive-complexity"
gendermap_t read_gender_file(const std::string& file_path, std::string& data) {

    gendermap_t ret;
    const auto e = my::utils::file_open_and_read_all(file_path, data);
    MYASSERT(e.code() == std::error_code(), e.what())

    const auto lines
        = my::utils::strings::split<std::string_view>(data, "\r\n");
    MYASSERT(lines.size() > 4, "Not enough lines in merge_from file")

    int line_number = 0;
    int warned_empty = 0;
    for (const auto& line : lines) {

        if (line.empty()) {
            if (warned_empty == 0) {
                cout << "NOTE: empty line in file: " << file_path << endl
                     << "at line: " << line_number
                     << ". There may be more, but I will not warn again."
                     << endl
                     << endl;
            }
            line_number++;
            ++warned_empty;
            continue; // millions of empty lines at the end of Simon's file for
                      // some reason
        }
        auto splut_line
            = my::utils::strings::split<std::string_view>(line, "\t");
        if (splut_line.size() != 2) {
            if (warned_empty == 0) {
                cerr << "Bad line in file: " << file_path << ", at line "
                     << line_number << " should be <ARTIST>TAB<GENDER>" << endl
                     << "There may be more, but I will not warn again." << endl
                     << endl;
            }
            ++line_number;
            ++warned_empty;
            continue;
        }

        const auto art = my::utils::strings::trim(splut_line[0]);
        const auto gen = my::utils::strings::trim(splut_line[1]);
        if (art.empty() && gen.empty()) {
            if (warned_empty == 0) {
                cout << "NOTE: both artist and gender are EMPTY: " << file_path
                     << endl
                     << "at line: " << line_number
                     << ". There may be more, but I will not warn again."
                     << endl
                     << endl;
            }
            line_number++;
            ++warned_empty;
            continue; // millions of empty lines at the end of Simon's file for
                      // some reason
        }
        if (art.empty()) {
            cerr << "Bad line in file: " << file_path << ", at line "
                 << line_number
                 << " both artist and gender should be specified:" << endl
                 << line << endl;
            ++line_number;
            continue;
        }

        if (gen.empty()) {
            cerr << "Bad line in file: " << file_path << ", at line "
                 << line_number << " gender not specified, for: " << art
                 << ", assuming female." << endl
                 << line << endl;
        }

        // we do it this way so whichever key was found LAST overrides earlier
        // entries.
        ret[art] = gender_from_string(gen);
        // must be good here
        ++line_number;
    }

    if (warned_empty > 0) {
        cout << "NOTE: There were " << warned_empty
             << " EMPTY or MALFORMED entries in file: " << file_path << endl
             << endl;
    }
    return ret;
}
#pragma clang diagnostic pop

using art_gender_pair = std::pair<std::string_view, gender_t>;

using diff_t = std::vector<std::pair<std::string_view, gender_t>>;

struct gender_t_cmp_less {
    bool operator()(
        const art_gender_pair& a, const art_gender_pair& b) const noexcept {
        return my::utils::strings::compare_less_nocase(a.first, b.first);
    }
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-easily-swappable-parameters"
gendermap_t merge(const gendermap_t& from, const gendermap_t& into,
    diff_t& new_ones, diff_t& kept_from_original) {

    gendermap_t ret = from;
    ret.insert(from.begin(), from.end());
    // from takes precedence, as it is the customer's file, so we do it second.
    ret.insert(into.begin(), into.end());

    const auto& a = from;
    const auto& b = into;

    std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
        std::back_inserter(new_ones), gender_t_cmp_less());

    auto& g = kept_from_original;
    std::set_difference(b.begin(), b.end(), a.begin(), a.end(),
        std::back_inserter(g), gender_t_cmp_less());

    return ret;
}
#pragma clang diagnostic pop

void print_results(const gendermap_t& merge_into_values, const diff_t& new_ones,
    const diff_t& kept_from_original, const gendermap_t& merged) {
    const auto difference = merged.size() - merge_into_values.size();

    const auto dif = new_ones.size();
    MYASSERT(difference == dif, "inconsistent sizes after merge")
    if (dif > 0) {
        cout << "During merge, there were " << new_ones.size()
             << " new entries, with precedence given to the values in:" << endl
             << g_source_file << endl
             << endl;

        size_t i = 0;
        constexpr size_t max_display_lines = 20;
        size_t samplesize
            = (new_ones.size() < max_display_lines ? new_ones.size()
                                                   : max_display_lines);
        cout << "****** Sample of new entries *******\n\n";

        for (const auto& pr : new_ones) {
            cout
                << pr.first << '\t'
                << genders[static_cast<
                       int>( // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                       pr.second)] // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                << '\n';
            if (i++ >= samplesize) {
                break;
            }
        }
        cout << "******* Sample of new entries ends *******" << endl << endl;

        ////////////////////////////////////////////
        cout << "****** Sample of old entries kept *******\n\n";
        samplesize = (kept_from_original.size() < max_display_lines
                ? kept_from_original.size()
                : max_display_lines);
        i = 0;

        for (const auto& pr : kept_from_original) {
            cout
                << pr.first << '\t'
                << genders[static_cast<
                       int>( // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                       pr.second)] // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                << '\n';
            if (i++ >= samplesize) {
                break;
            }
        }
        cout << "******** Sample of old entries kept ends *******" << endl
             << endl;
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored                                                 \
    "cppcoreguidelines-pro-bounds-constant-array-index"
int mymain(const int argc, char** argv) {

    parseArgs(argc, argv);

    const auto merge_from_values
        = read_gender_file(g_source_file, g_merge_from_data);
    cout << "Input (source) file " << g_source_file << endl
         << "has " << merge_from_values.size() << " useful entries." << endl
         << endl;

    const auto merge_into_values
        = read_gender_file(g_dest_file, g_merge_into_data);
    cout << "Output (dest) file " << g_dest_file << endl
         << "has " << merge_into_values.size()
         << " existing useful entries before merge." << endl
         << endl;

    diff_t new_ones;
    diff_t kept_from_original;

    const auto merged = merge(
        merge_from_values, merge_into_values, new_ones, kept_from_original);

    print_results(merge_into_values, new_ones, kept_from_original, merged);

    cout << "Merge will result in new file:\n"
         << g_dest_file << "\nwith " << merged.size() << " entries." << endl
         << endl;

    const auto backup_path = my::utils::get_temp_filename();
    const auto e = my::utils::file_move(g_dest_file, backup_path);
    assert(e.code() == std::error_code());

    std::fstream f;
    const auto error = my::utils::file_open(
        f, g_dest_file, std::ios::out | std::ios::binary);

    for (const auto& pr : merged) {
        std::string s{pr.first};
        s += '\t';
        s += genders[static_cast<int>(pr.second)];
        s += "\r\n";
        f.write(s.data(),
            static_cast<std::streamsize>(
                s.size())); // NOLINT(bugprone-narrowing-conversions)
    }
    assert(f);
    f.close();

    return 0;
}
#pragma clang diagnostic pop

int main(const int argc, char** argv) {

    try {
        my::stopwatch sw("merging gender files took: ");
        return mymain(argc, argv);
    } catch (const std::exception& e) {
        cerr << e.what() << endl;
    }
}

#pragma clang diagnostic pop
