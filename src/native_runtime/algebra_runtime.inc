// Abstract algebra (std.math.algebra): the ring Z/nZ, prime fields GF(p), polynomial rings GF(p)[x],
// and finite fields GF(p^k).
//
// Residues are Longs in [0, n). Products use 128-bit intermediates, so any modulus below 2^63 is
// exact. An element of GF(p^k) = GF(p)[x]/(m) is packed into one Long as the base-p digits of its
// coefficients (a0 + a1*p + a2*p^2 + ...); the field's monic modulus m is packed the same way, so
// p^(k+1) must stay below 2^62. Polynomials of any degree are runtime handles (never freed; the
// handle table grows with every polynomial produced). Misuse -- a non-prime field characteristic,
// mixing elements of different rings, dividing by a non-unit -- prints "AlgebraError: ..." and
// exits with status 1.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace csec_algebra {

using i64 = long long;
using u64 = unsigned long long;

[[noreturn]] void fail(const std::string& message) {
    std::fflush(stdout);
    std::fprintf(stderr, "AlgebraError: %s\n", message.c_str());
    std::fflush(stderr);
    std::exit(1);
}

i64 norm(i64 a, i64 n) {
    if (n <= 0) fail("modulus must be positive, got " + std::to_string(n));
    i64 r = a % n;
    return r < 0 ? r + n : r;
}

u64 mulmodU(u64 a, u64 b, u64 n) {
#if defined(_MSC_VER)
    u64 high = 0;
    const u64 low = _umul128(a, b, &high);
    u64 remainder = 0;
    (void)_udiv128(high % n, low, n, &remainder);
    return remainder;
#else
    return static_cast<u64>((static_cast<unsigned __int128>(a) * b) % n);
#endif
}

i64 mulmod(i64 a, i64 b, i64 n) {
    return static_cast<i64>(mulmodU(static_cast<u64>(norm(a, n)), static_cast<u64>(norm(b, n)), static_cast<u64>(n)));
}

i64 addmod(i64 a, i64 b, i64 n) {
    const u64 s = static_cast<u64>(norm(a, n)) + static_cast<u64>(norm(b, n));
    return static_cast<i64>(s % static_cast<u64>(n));
}

i64 submod(i64 a, i64 b, i64 n) {
    return addmod(a, n - norm(b, n), n);
}

i64 gcd(i64 a, i64 b) {
    a = a < 0 ? -a : a;
    b = b < 0 ? -b : b;
    while (b != 0) {
        const i64 t = a % b;
        a = b;
        b = t;
    }
    return a;
}

// Inverse of a mod n, or -1 when gcd(a, n) != 1.
i64 tryInverse(i64 a, i64 n) {
    a = norm(a, n);
    if (n == 1) return 0;
    i64 oldR = a, r = n, oldS = 1, s = 0;
    while (r != 0) {
        const i64 q = oldR / r;
        i64 t = oldR - q * r; oldR = r; r = t;
        t = oldS - q * s; oldS = s; s = t;
    }
    if (oldR != 1) return -1;
    return norm(oldS, n);
}

i64 inverse(i64 a, i64 n) {
    const i64 inv = tryInverse(a, n);
    if (inv < 0) fail(std::to_string(norm(a, n)) + " has no inverse modulo " + std::to_string(n));
    return inv;
}

i64 powmod(i64 a, i64 e, i64 n) {
    if (e < 0) {
        a = inverse(a, n);
        e = -e;
    }
    u64 result = static_cast<u64>(1 % n);
    u64 base = static_cast<u64>(norm(a, n));
    u64 k = static_cast<u64>(e);
    while (k > 0) {
        if (k & 1) result = mulmodU(result, base, static_cast<u64>(n));
        base = mulmodU(base, base, static_cast<u64>(n));
        k >>= 1;
    }
    return static_cast<i64>(result);
}

bool isPrime(i64 n) {
    if (n < 2) return false;
    for (i64 small : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
        if (n % small == 0) return n == small;
    }
    i64 d = n - 1;
    int s = 0;
    while ((d & 1) == 0) { d >>= 1; ++s; }
    // Deterministic for every 64-bit n with these bases.
    for (i64 a : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
        i64 x = powmod(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool composite = true;
        for (int r = 1; r < s; ++r) {
            x = mulmod(x, x, n);
            if (x == n - 1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

std::vector<i64> primeFactors(i64 n) {
    std::vector<i64> factors;
    for (i64 f = 2; f * f <= n; ++f) {
        if (n % f == 0) {
            factors.push_back(f);
            while (n % f == 0) n /= f;
        }
    }
    if (n > 1) factors.push_back(n);
    return factors;
}

i64 eulerPhi(i64 n) {
    i64 result = n;
    for (i64 f : primeFactors(n)) result = result / f * (f - 1);
    return result;
}

// Multiplicative order of a unit a mod n.
i64 order(i64 a, i64 n) {
    a = norm(a, n);
    if (gcd(a, n) != 1) fail(std::to_string(a) + " is not a unit modulo " + std::to_string(n) + "; it has no multiplicative order");
    i64 result = eulerPhi(n);
    for (i64 f : primeFactors(result)) {
        while (result % f == 0 && powmod(a, result / f, n) == 1) result /= f;
    }
    return result;
}

i64 legendre(i64 a, i64 p) {
    a = norm(a, p);
    if (a == 0) return 0;
    if (p == 2) return 1;
    return powmod(a, (p - 1) / 2, p) == 1 ? 1 : -1;
}

// Tonelli-Shanks; -1 when a is not a square mod p.
i64 sqrtmod(i64 a, i64 p) {
    a = norm(a, p);
    if (a == 0 || p == 2) return a;
    if (legendre(a, p) != 1) return -1;
    i64 q = p - 1;
    int s = 0;
    while ((q & 1) == 0) { q >>= 1; ++s; }
    i64 z = 2;
    while (legendre(z, p) != -1) ++z;
    i64 m = s, c = powmod(z, q, p), t = powmod(a, q, p), r = powmod(a, (q + 1) / 2, p);
    while (t != 1) {
        i64 i = 0, t2 = t;
        while (t2 != 1) { t2 = mulmod(t2, t2, p); ++i; }
        i64 b = c;
        for (i64 j = 0; j < m - i - 1; ++j) b = mulmod(b, b, p);
        m = i;
        c = mulmod(b, b, p);
        t = mulmod(t, c, p);
        r = mulmod(r, b, p);
    }
    const i64 other = p - r;
    return r < other ? r : other;
}

i64 primitiveRoot(i64 p) {
    if (p == 2) return 1;
    const auto factors = primeFactors(p - 1);
    for (i64 g = 2; g < p; ++g) {
        bool generator = true;
        for (i64 f : factors) {
            if (powmod(g, (p - 1) / f, p) == 1) { generator = false; break; }
        }
        if (generator) return g;
    }
    return 1;
}

// ---- polynomials over GF(p): coefficient vectors, lowest degree first, no trailing zeros ----

using Poly = std::vector<i64>;

void trim(Poly& a) {
    while (!a.empty() && a.back() == 0) a.pop_back();
}

int degree(const Poly& a) { return static_cast<int>(a.size()) - 1; }

Poly polyAdd(const Poly& a, const Poly& b, i64 p) {
    Poly r(a.size() > b.size() ? a.size() : b.size(), 0);
    for (size_t i = 0; i < r.size(); ++i) {
        r[i] = addmod(i < a.size() ? a[i] : 0, i < b.size() ? b[i] : 0, p);
    }
    trim(r);
    return r;
}

Poly polySub(const Poly& a, const Poly& b, i64 p) {
    Poly r(a.size() > b.size() ? a.size() : b.size(), 0);
    for (size_t i = 0; i < r.size(); ++i) {
        r[i] = submod(i < a.size() ? a[i] : 0, i < b.size() ? b[i] : 0, p);
    }
    trim(r);
    return r;
}

Poly polyScale(const Poly& a, i64 k, i64 p) {
    Poly r(a.size(), 0);
    for (size_t i = 0; i < a.size(); ++i) r[i] = mulmod(a[i], k, p);
    trim(r);
    return r;
}

Poly polyMul(const Poly& a, const Poly& b, i64 p) {
    if (a.empty() || b.empty()) return {};
    Poly r(a.size() + b.size() - 1, 0);
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] == 0) continue;
        for (size_t j = 0; j < b.size(); ++j) r[i + j] = addmod(r[i + j], mulmod(a[i], b[j], p), p);
    }
    trim(r);
    return r;
}

// a = q*b + r with deg r < deg b.
void polyDivMod(const Poly& a, const Poly& b, i64 p, Poly& q, Poly& r) {
    if (b.empty()) fail("polynomial division by zero");
    r = a;
    q.assign(a.size() >= b.size() ? a.size() - b.size() + 1 : 0, 0);
    const i64 leadInverse = inverse(b.back(), p);
    while (!r.empty() && r.size() >= b.size()) {
        const size_t shift = r.size() - b.size();
        const i64 factor = mulmod(r.back(), leadInverse, p);
        q[shift] = factor;
        for (size_t i = 0; i < b.size(); ++i) r[shift + i] = submod(r[shift + i], mulmod(factor, b[i], p), p);
        trim(r);
    }
    trim(q);
}

Poly polyMod(const Poly& a, const Poly& m, i64 p) {
    Poly q, r;
    polyDivMod(a, m, p, q, r);
    return r;
}

Poly polyMonic(const Poly& a, i64 p) {
    if (a.empty()) return a;
    return polyScale(a, inverse(a.back(), p), p);
}

Poly polyGcd(Poly a, Poly b, i64 p) {
    while (!b.empty()) {
        Poly r = polyMod(a, b, p);
        a = b;
        b = r;
    }
    return polyMonic(a, p);
}

Poly polyPowMod(Poly base, u64 e, const Poly& m, i64 p) {
    Poly result{1 % p};
    trim(result);
    base = polyMod(base, m, p);
    while (e > 0) {
        if (e & 1) result = polyMod(polyMul(result, base, p), m, p);
        base = polyMod(polyMul(base, base, p), m, p);
        e >>= 1;
    }
    return result;
}

// x^(p^count) mod f, by repeated p-th powering.
Poly frobenius(const Poly& f, i64 p, int count) {
    Poly x{0, 1};
    Poly r = polyMod(x, f, p);
    for (int i = 0; i < count; ++i) r = polyPowMod(r, static_cast<u64>(p), f, p);
    return r;
}

// Rabin's irreducibility test.
bool isIrreducible(const Poly& fIn, i64 p) {
    const Poly f = polyMonic(fIn, p);
    const int n = degree(f);
    if (n < 1) return false;
    if (n == 1) return true;
    const Poly x{0, 1};
    if (!polySub(frobenius(f, p, n), polyMod(x, f, p), p).empty()) return false;
    for (i64 q : primeFactors(n)) {
        const Poly h = polySub(frobenius(f, p, static_cast<int>(n / q)), polyMod(x, f, p), p);
        if (degree(polyGcd(f, h, p)) != 0) return false;
    }
    return true;
}

i64 polyEval(const Poly& a, i64 x, i64 p) {
    i64 r = 0;
    for (size_t i = a.size(); i-- > 0;) r = addmod(mulmod(r, x, p), a[i], p);
    return r;
}

std::string polyToString(const Poly& a, const char* variable) {
    if (a.empty()) return "0";
    std::string out;
    for (size_t i = a.size(); i-- > 0;) {
        if (a[i] == 0) continue;
        if (!out.empty()) out += " + ";
        const bool unitCoefficient = a[i] == 1 && i > 0;
        if (!unitCoefficient) out += std::to_string(a[i]);
        if (i >= 1) out += variable;
        if (i >= 2) out += "^" + std::to_string(i);
    }
    return out;
}

// ---- packing GF(p^k) elements into Longs ----

Poly unpack(i64 packed, i64 p) {
    Poly a;
    u64 v = static_cast<u64>(packed);
    while (v > 0) {
        a.push_back(static_cast<i64>(v % static_cast<u64>(p)));
        v /= static_cast<u64>(p);
    }
    trim(a);
    return a;
}

i64 pack(const Poly& a, i64 p) {
    u64 v = 0;
    for (size_t i = a.size(); i-- > 0;) v = v * static_cast<u64>(p) + static_cast<u64>(a[i]);
    return static_cast<i64>(v);
}

// p^(k+1) must fit in 62 bits so that a packed degree-k modulus is representable.
void checkPackable(i64 p, i64 k) {
    long double size = 1;
    for (i64 i = 0; i <= k; ++i) size *= static_cast<long double>(p);
    if (size > 4.0e18L) {
        fail("GF(" + std::to_string(p) + "^" + std::to_string(k) + ") is too large; p^(k+1) must stay below 2^62");
    }
}

// ---- polynomial handles ----

struct PolyEntry {
    i64 p = 2;
    Poly coefficients;
};

std::mutex& polyMutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector<PolyEntry>& polys() {
    static auto* table = new std::vector<PolyEntry>();
    return *table;
}

i64 store(i64 p, Poly coefficients) {
    trim(coefficients);
    std::lock_guard<std::mutex> lock(polyMutex());
    polys().push_back(PolyEntry{p, std::move(coefficients)});
    return static_cast<i64>(polys().size());
}

PolyEntry load(i64 handle) {
    std::lock_guard<std::mutex> lock(polyMutex());
    if (handle < 1 || static_cast<size_t>(handle) > polys().size()) fail("invalid polynomial");
    return polys()[static_cast<size_t>(handle - 1)];
}

i64 sameCharacteristic(const PolyEntry& a, const PolyEntry& b) {
    if (a.p != b.p) {
        fail("polynomials over GF(" + std::to_string(a.p) + ") and GF(" + std::to_string(b.p) + ") cannot be combined");
    }
    return a.p;
}

char* ownedString(const std::string& text) {
    char* out = static_cast<char*>(std::malloc(text.size() + 1));
    if (out) std::memcpy(out, text.c_str(), text.size() + 1);
    return out;
}

} // namespace csec_algebra

extern "C" {

// ---- integers modulo n ----

long long csec_alg_mod(long long a, long long n) { return csec_algebra::norm(a, n); }
long long csec_alg_add(long long a, long long b, long long n) { return csec_algebra::addmod(a, b, n); }
long long csec_alg_sub(long long a, long long b, long long n) { return csec_algebra::submod(a, b, n); }
long long csec_alg_mul(long long a, long long b, long long n) { return csec_algebra::mulmod(a, b, n); }
long long csec_alg_pow(long long a, long long e, long long n) { return csec_algebra::powmod(a, e, n); }
long long csec_alg_inv(long long a, long long n) { return csec_algebra::inverse(a, n); }
int csec_alg_is_unit(long long a, long long n) { return csec_algebra::tryInverse(a, n) >= 0 ? 1 : 0; }
long long csec_alg_gcd(long long a, long long b) { return csec_algebra::gcd(a, b); }
int csec_alg_is_prime(long long n) { return csec_algebra::isPrime(n) ? 1 : 0; }
long long csec_alg_euler_phi(long long n) {
    if (n < 1) csec_algebra::fail("eulerPhi needs a positive integer");
    return csec_algebra::eulerPhi(n);
}
long long csec_alg_order(long long a, long long n) { return csec_algebra::order(a, n); }
long long csec_alg_legendre(long long a, long long p) { return csec_algebra::legendre(a, p); }
long long csec_alg_sqrt(long long a, long long p) {
    const long long root = csec_algebra::sqrtmod(a, p);
    if (root < 0) {
        csec_algebra::fail(std::to_string(csec_algebra::norm(a, p)) + " is not a square in GF(" + std::to_string(p) + ")");
    }
    return root;
}
int csec_alg_is_square(long long a, long long p) { return csec_algebra::sqrtmod(a, p) >= 0 ? 1 : 0; }
long long csec_alg_primitive_root(long long p) { return csec_algebra::primitiveRoot(p); }

long long csec_alg_require_modulus(long long n) {
    if (n < 1) csec_algebra::fail("Z/nZ needs a modulus n >= 1, got " + std::to_string(n));
    return n;
}

long long csec_alg_require_prime(long long p) {
    // GF<P> checks its prime on every operation; remember the primes already proved.
    static std::mutex cacheMutex;
    static std::vector<long long> proved;
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        for (long long known : proved) if (known == p) return p;
    }
    if (csec_algebra::isPrime(p)) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (proved.size() < 64) proved.push_back(p);
        return p;
    }
    {
        std::string hint;
        if (p > 1) {
            const auto factors = csec_algebra::primeFactors(p);
            if (factors.size() == 1) {
                long long k = 0;
                for (long long q = p; q > 1; q /= factors[0]) ++k;
                hint = "; the field with " + std::to_string(p) + " elements is galoisField(" +
                       std::to_string(factors[0]) + ", " + std::to_string(k) + ")";
            }
        }
        csec_algebra::fail("GF(" + std::to_string(p) + ") is not a prime field: " + std::to_string(p) +
                           " is not prime" + hint);
    }
    return p;
}

long long csec_alg_require_same(long long a, long long b) {
    if (a != b) {
        csec_algebra::fail("elements of different rings cannot be combined (modulus " + std::to_string(a) +
                           " vs " + std::to_string(b) + ")");
    }
    return a;
}

char* csec_alg_residue_to_string(long long v, long long n) {
    return csec_algebra::ownedString(std::to_string(v) + " (mod " + std::to_string(n) + ")");
}

// ---- polynomials over GF(p) ----

long long csec_poly_constant(long long c, long long p) {
    return csec_algebra::store(p, csec_algebra::Poly{csec_algebra::norm(c, p)});
}

long long csec_poly_monomial(long long c, long long k, long long p) {
    if (k < 0) csec_algebra::fail("a monomial needs a non-negative exponent");
    csec_algebra::Poly a(static_cast<size_t>(k) + 1, 0);
    a[static_cast<size_t>(k)] = csec_algebra::norm(c, p);
    return csec_algebra::store(p, a);
}

long long csec_poly_characteristic(long long h) { return csec_algebra::load(h).p; }

long long csec_poly_add(long long a, long long b) {
    auto x = csec_algebra::load(a), y = csec_algebra::load(b);
    const auto p = csec_algebra::sameCharacteristic(x, y);
    return csec_algebra::store(p, csec_algebra::polyAdd(x.coefficients, y.coefficients, p));
}

long long csec_poly_sub(long long a, long long b) {
    auto x = csec_algebra::load(a), y = csec_algebra::load(b);
    const auto p = csec_algebra::sameCharacteristic(x, y);
    return csec_algebra::store(p, csec_algebra::polySub(x.coefficients, y.coefficients, p));
}

long long csec_poly_mul(long long a, long long b) {
    auto x = csec_algebra::load(a), y = csec_algebra::load(b);
    const auto p = csec_algebra::sameCharacteristic(x, y);
    return csec_algebra::store(p, csec_algebra::polyMul(x.coefficients, y.coefficients, p));
}

long long csec_poly_scale(long long a, long long k) {
    auto x = csec_algebra::load(a);
    return csec_algebra::store(x.p, csec_algebra::polyScale(x.coefficients, csec_algebra::norm(k, x.p), x.p));
}

long long csec_poly_div(long long a, long long b) {
    auto x = csec_algebra::load(a), y = csec_algebra::load(b);
    const auto p = csec_algebra::sameCharacteristic(x, y);
    csec_algebra::Poly q, r;
    csec_algebra::polyDivMod(x.coefficients, y.coefficients, p, q, r);
    return csec_algebra::store(p, q);
}

long long csec_poly_rem(long long a, long long b) {
    auto x = csec_algebra::load(a), y = csec_algebra::load(b);
    const auto p = csec_algebra::sameCharacteristic(x, y);
    return csec_algebra::store(p, csec_algebra::polyMod(x.coefficients, y.coefficients, p));
}

long long csec_poly_pow(long long a, long long e) {
    if (e < 0) csec_algebra::fail("a polynomial power needs a non-negative exponent");
    auto x = csec_algebra::load(a);
    csec_algebra::Poly result{1 % x.p};
    csec_algebra::Poly base = x.coefficients;
    while (e > 0) {
        if (e & 1) result = csec_algebra::polyMul(result, base, x.p);
        base = csec_algebra::polyMul(base, base, x.p);
        e >>= 1;
    }
    return csec_algebra::store(x.p, result);
}

long long csec_poly_gcd(long long a, long long b) {
    auto x = csec_algebra::load(a), y = csec_algebra::load(b);
    const auto p = csec_algebra::sameCharacteristic(x, y);
    return csec_algebra::store(p, csec_algebra::polyGcd(x.coefficients, y.coefficients, p));
}

long long csec_poly_derivative(long long a) {
    auto x = csec_algebra::load(a);
    csec_algebra::Poly d;
    for (size_t i = 1; i < x.coefficients.size(); ++i) {
        d.push_back(csec_algebra::mulmod(x.coefficients[i], static_cast<long long>(i), x.p));
    }
    return csec_algebra::store(x.p, d);
}

long long csec_poly_monic(long long a) {
    auto x = csec_algebra::load(a);
    return csec_algebra::store(x.p, csec_algebra::polyMonic(x.coefficients, x.p));
}

int csec_poly_degree(long long a) { return csec_algebra::degree(csec_algebra::load(a).coefficients); }

long long csec_poly_coeff(long long a, long long i) {
    auto x = csec_algebra::load(a);
    return i >= 0 && static_cast<size_t>(i) < x.coefficients.size() ? x.coefficients[static_cast<size_t>(i)] : 0;
}

long long csec_poly_eval(long long a, long long v) {
    auto x = csec_algebra::load(a);
    return csec_algebra::polyEval(x.coefficients, csec_algebra::norm(v, x.p), x.p);
}

int csec_poly_equals(long long a, long long b) {
    auto x = csec_algebra::load(a), y = csec_algebra::load(b);
    return x.p == y.p && x.coefficients == y.coefficients ? 1 : 0;
}

int csec_poly_is_irreducible(long long a) {
    auto x = csec_algebra::load(a);
    return csec_algebra::isIrreducible(x.coefficients, x.p) ? 1 : 0;
}

char* csec_poly_to_string(long long a) {
    return csec_algebra::ownedString(csec_algebra::polyToString(csec_algebra::load(a).coefficients, "x"));
}

// ---- finite fields GF(p^k): packed elements ----

// The smallest monic irreducible polynomial of degree k over GF(p), packed.
long long csec_gf_find_modulus(long long p, long long k) {
    csec_alg_require_prime(p);
    if (k < 1) csec_algebra::fail("GF(p^k) needs k >= 1");
    csec_algebra::checkPackable(p, k);
    long long start = 1;
    for (long long i = 0; i < k; ++i) start *= p;  // x^k
    for (long long candidate = start; candidate < 2 * start; ++candidate) {
        if (csec_algebra::isIrreducible(csec_algebra::unpack(candidate, p), p)) return candidate;
    }
    csec_algebra::fail("no irreducible polynomial found");
}

// A user-chosen modulus (a polynomial handle): must be irreducible; it is made monic and packed.
long long csec_gf_modulus_from_poly(long long h) {
    auto x = csec_algebra::load(h);
    csec_alg_require_prime(x.p);
    const auto k = csec_algebra::degree(x.coefficients);
    if (k < 1 || !csec_algebra::isIrreducible(x.coefficients, x.p)) {
        csec_algebra::fail(csec_algebra::polyToString(x.coefficients, "x") + " is not irreducible over GF(" +
                           std::to_string(x.p) + "), so the quotient is not a field");
    }
    csec_algebra::checkPackable(x.p, k);
    return csec_algebra::pack(csec_algebra::polyMonic(x.coefficients, x.p), x.p);
}

long long csec_gf_degree(long long m, long long p) { return csec_algebra::degree(csec_algebra::unpack(m, p)); }

long long csec_gf_order(long long m, long long p) {
    long long q = 1;
    for (long long i = 0; i < csec_gf_degree(m, p); ++i) q *= p;
    return q;
}

long long csec_gf_reduce(long long a, long long p, long long m) {
    return csec_algebra::pack(csec_algebra::polyMod(csec_algebra::unpack(a, p), csec_algebra::unpack(m, p), p), p);
}

long long csec_gf_from_int(long long v, long long p) { return csec_algebra::norm(v, p); }

long long csec_gf_add(long long a, long long b, long long p) {
    return csec_algebra::pack(csec_algebra::polyAdd(csec_algebra::unpack(a, p), csec_algebra::unpack(b, p), p), p);
}

long long csec_gf_sub(long long a, long long b, long long p) {
    return csec_algebra::pack(csec_algebra::polySub(csec_algebra::unpack(a, p), csec_algebra::unpack(b, p), p), p);
}

long long csec_gf_mul(long long a, long long b, long long p, long long m) {
    const auto product = csec_algebra::polyMul(csec_algebra::unpack(a, p), csec_algebra::unpack(b, p), p);
    return csec_algebra::pack(csec_algebra::polyMod(product, csec_algebra::unpack(m, p), p), p);
}

long long csec_gf_pow(long long a, long long e, long long p, long long m) {
    const long long q = csec_gf_order(m, p);
    if (a == 0) {
        if (e < 0) csec_algebra::fail("0 has no inverse in GF(" + std::to_string(q) + ")");
        return e == 0 ? 1 : 0;
    }
    // The multiplicative group has order q - 1.
    long long k = e % (q - 1);
    if (k < 0) k += q - 1;
    return csec_algebra::pack(csec_algebra::polyPowMod(csec_algebra::unpack(a, p), static_cast<unsigned long long>(k),
                                                       csec_algebra::unpack(m, p), p), p);
}

long long csec_gf_inv(long long a, long long p, long long m) {
    if (a == 0) csec_algebra::fail("0 has no inverse in GF(" + std::to_string(csec_gf_order(m, p)) + ")");
    return csec_gf_pow(a, -1, p, m);
}

long long csec_gf_from_poly(long long h, long long p, long long m) {
    auto x = csec_algebra::load(h);
    csec_alg_require_same(x.p, p);
    return csec_algebra::pack(csec_algebra::polyMod(x.coefficients, csec_algebra::unpack(m, p), p), p);
}

long long csec_gf_to_poly(long long a, long long p) { return csec_algebra::store(p, csec_algebra::unpack(a, p)); }

// The multiplicative order of a nonzero element.
long long csec_gf_element_order(long long a, long long p, long long m) {
    if (a == 0) csec_algebra::fail("0 has no multiplicative order");
    const long long groupOrder = csec_gf_order(m, p) - 1;
    long long result = groupOrder;
    for (long long f : csec_algebra::primeFactors(groupOrder)) {
        while (result % f == 0 && csec_gf_pow(a, result / f, p, m) == 1) result /= f;
    }
    return result;
}

// A generator of the multiplicative group (the smallest packed one).
long long csec_gf_primitive_element(long long p, long long m) {
    const long long q = csec_gf_order(m, p);
    for (long long a = 1; a < q; ++a) {
        if (csec_gf_element_order(a, p, m) == q - 1) return a;
    }
    return 1;
}

char* csec_gf_to_string(long long a, long long p) {
    return csec_algebra::ownedString(csec_algebra::polyToString(csec_algebra::unpack(a, p), "a"));
}

} // extern "C"
