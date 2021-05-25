
#include <fstream>
#include <cfloat>
#include <glm/geometric.hpp>

#include "Utils.h"

#define EPSILON 0.0001

using namespace std;
using namespace glm;

string utils::ReadFile(const char* path)
{
    ifstream fin(path);
    return string((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
}

bool utils::LineIntersectsPlane(vec3 lp, vec3 ld, vec3 pp, vec3 pn, vec3& intersection)
{
    float numer = dot(pp - lp, pn);

    if (abs(numer) <= FLT_EPSILON) // Line is on the plane
        return false;

    float denom = dot(ld, pn);

    if (abs(denom) <= FLT_EPSILON) // Line is parallel to plane
        return false;

    float d = numer / denom;

    intersection = lp + d * ld;
    return true;
}

bool utils::IsBetween(float x, float a, float b)
{
    if (a < b)
        return x >= a && x <= b;

    if (a > b)
        return x >= b && x <= a;

    return false;
}

bool utils::ApproxEquals(float a, float b)
{
    return ApproxEquals(a, b, EPSILON);
}

bool utils::ApproxEquals(float a, float b, float epsilon)
{
    return abs(a - b) <= epsilon;
}

bool utils::LineIntersectsRect(vec3 lp, vec3 ld, vec3 tl, vec3 tr, vec3 bl, vec3 br, vec3& intersection)
{
    vec3 h = bl + 0.5f * (tl - bl);
    vec3 center = h + 0.5f * (tr - tl);
    vec3 normal = normalize(cross(tl - br, tr - bl));

    vec3 x;
    bool intersects_inf_plane = LineIntersectsPlane(lp, ld, center, normal, x);
    if (!intersects_inf_plane)
        return false;

    vec3 u = tl - tr;
    vec3 v = tl - bl;

    float dotu = dot(u, x);
    float dotv = dot(v, x);

    float dotutl = dot(u, tl);
    float dotutr = dot(u, tr);
    float dotvtl = dot(v, tl);
    float dotvbl = dot(v, bl);

    if (IsBetween(dotu, dot(u, tl), dot(u, tr))
        && IsBetween(dotv, dot(v, tl), dot(v, bl)))
    {
        intersection = x;
        return true;
    }

    return false;
}

vec3 NearerPoint(vec3 ref, vec3 a, vec3 b)
{
    if (length(ref - a) < length(ref - b))
        return a;

    return b;
}

bool utils::LineIntersectsBox(glm::vec3 lp, glm::vec3 ld, glm::vec3 ttl, glm::vec3 ttr, glm::vec3 tbl, glm::vec3 tbr, glm::vec3 btl, glm::vec3 btr, glm::vec3 bbl, glm::vec3 bbr, glm::vec3& intersection)
{
#define CHECK_AND_RETURN if (cnt == 2) { intersection = NearerPoint(lp, intersects[0], intersects[1]); return true; }

    int cnt = 0;
    vec3 intersects[2];

    vec3 i;
    if (LineIntersectsRect(lp, ld, tbl, tbr, bbl, bbr, i))
        intersects[cnt++] = i;

    if (LineIntersectsRect(lp, ld, ttl, ttr, btl, btr, i))
        intersects[cnt++] = i;

    CHECK_AND_RETURN

        if (LineIntersectsRect(lp, ld, ttl, tbl, btl, bbl, i))
            intersects[cnt++] = i;

    CHECK_AND_RETURN

        if (LineIntersectsRect(lp, ld, tbr, ttr, bbr, btr, i))
            intersects[cnt++] = i;

    CHECK_AND_RETURN

        if (LineIntersectsRect(lp, ld, ttl, ttr, tbl, tbr, i))
            intersects[cnt++] = i;

    CHECK_AND_RETURN

        if (LineIntersectsRect(lp, ld, btl, btr, bbl, bbr, i))
            intersects[cnt++] = i;

    CHECK_AND_RETURN

        return false;

#undef CHECK_AND_RETURN
}


bool utils::StringStartsWith(const string& src, const char* start)
{
    string prefix(start);
    if (prefix.length() > src.length()) return false;
    return equal(prefix.begin(), prefix.end(), src.begin());
}

bool utils::StringEndsWith(const string& src, const char* end)
{
    string suffix(end);
    if (suffix.length() > src.length()) return false;
    return equal(suffix.rbegin(), suffix.rend(), src.rbegin());
}


bool utils::StringEquals(const string& src, const char* other)
{
    return src.compare(other) == 0;
}

int utils::ParseNumericString(const std::string& src)
{
    if (StringEndsWith(src, "h"))
        return stoi(src.substr(0, src.length() - 1), nullptr, 16);

    return stoi(src);
}
