// ============================================================================
//  bigint_runtime.inc  --  arbitrary-precision integers for the Nat type
//
//  A Nat (Gaussian integer) stores two of these as its real and imaginary
//  components, so a natural number has no 64-bit ceiling: its magnitude grows
//  with the value.  Representation is sign-magnitude, little-endian, base 1e9
//  (each limb holds 0..999999999, which keeps a full limb*limb product inside
//  a uint64_t and makes decimal formatting a plain per-limb print).
//
//  Handles are heap objects returned as void* and are caller-owned / leaked,
//  matching the char* string helpers elsewhere in the runtime.
// ============================================================================

namespace {

constexpr std::uint32_t CSEC_BIGINT_BASE = 1000000000u;  // 1e9

struct CsecBigInt {
    int sign = 0;                       // -1, 0, +1  (0 => the value is zero)
    std::vector<std::uint32_t> mag;     // base-1e9 limbs, little-endian, no trailing zeros
};

void csecBigIntTrim(std::vector<std::uint32_t>& m) {
    while (!m.empty() && m.back() == 0) m.pop_back();
}

CsecBigInt* csecBigIntAlloc(int sign, std::vector<std::uint32_t> mag) {
    csecBigIntTrim(mag);
    auto* r = new CsecBigInt();
    r->mag = std::move(mag);
    r->sign = r->mag.empty() ? 0 : sign;   // canonical zero has sign 0
    return r;
}

// Compare magnitudes only: -1 (a<b), 0 (a==b), +1 (a>b).
int csecMagCmp(const std::vector<std::uint32_t>& a, const std::vector<std::uint32_t>& b) {
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    for (std::size_t i = a.size(); i-- > 0;) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

std::vector<std::uint32_t> csecMagAdd(const std::vector<std::uint32_t>& a, const std::vector<std::uint32_t>& b) {
    std::vector<std::uint32_t> r;
    std::size_t n = a.size() > b.size() ? a.size() : b.size();  // std::max clashes with windows.h max macro
    r.reserve(n + 1);
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < n; ++i) {
        std::uint64_t s = carry;
        if (i < a.size()) s += a[i];
        if (i < b.size()) s += b[i];
        r.push_back(static_cast<std::uint32_t>(s % CSEC_BIGINT_BASE));
        carry = s / CSEC_BIGINT_BASE;
    }
    if (carry) r.push_back(static_cast<std::uint32_t>(carry));
    return r;
}

// Magnitude subtraction, requires |a| >= |b|.
std::vector<std::uint32_t> csecMagSub(const std::vector<std::uint32_t>& a, const std::vector<std::uint32_t>& b) {
    std::vector<std::uint32_t> r;
    r.reserve(a.size());
    std::int64_t borrow = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        std::int64_t s = static_cast<std::int64_t>(a[i]) - borrow
            - (i < b.size() ? static_cast<std::int64_t>(b[i]) : 0);
        if (s < 0) { s += CSEC_BIGINT_BASE; borrow = 1; } else { borrow = 0; }
        r.push_back(static_cast<std::uint32_t>(s));
    }
    csecBigIntTrim(r);
    return r;
}

std::vector<std::uint32_t> csecMagMul(const std::vector<std::uint32_t>& a, const std::vector<std::uint32_t>& b) {
    if (a.empty() || b.empty()) return {};
    std::vector<std::uint64_t> acc(a.size() + b.size(), 0);
    for (std::size_t i = 0; i < a.size(); ++i) {
        std::uint64_t carry = 0;
        for (std::size_t j = 0; j < b.size(); ++j) {
            std::uint64_t cur = acc[i + j] + static_cast<std::uint64_t>(a[i]) * b[j] + carry;
            acc[i + j] = cur % CSEC_BIGINT_BASE;
            carry = cur / CSEC_BIGINT_BASE;
        }
        acc[i + b.size()] += carry;
    }
    std::vector<std::uint32_t> r(acc.size());
    for (std::size_t i = 0; i < acc.size(); ++i) r[i] = static_cast<std::uint32_t>(acc[i]);
    csecBigIntTrim(r);
    return r;
}

// Signed add of two big integers (subtraction is add-with-negated-b).
CsecBigInt* csecBigIntAddSigned(const CsecBigInt* a, const CsecBigInt* b) {
    if (a->sign == 0) return csecBigIntAlloc(b->sign, b->mag);
    if (b->sign == 0) return csecBigIntAlloc(a->sign, a->mag);
    if (a->sign == b->sign) return csecBigIntAlloc(a->sign, csecMagAdd(a->mag, b->mag));
    int c = csecMagCmp(a->mag, b->mag);
    if (c == 0) return csecBigIntAlloc(0, {});
    if (c > 0) return csecBigIntAlloc(a->sign, csecMagSub(a->mag, b->mag));
    return csecBigIntAlloc(b->sign, csecMagSub(b->mag, a->mag));
}

// Multiply a magnitude by a small factor (< base).
std::vector<std::uint32_t> csecMagMulSmall(const std::vector<std::uint32_t>& a, std::uint32_t factor) {
    if (a.empty() || factor == 0) return {};
    std::vector<std::uint32_t> r;
    r.reserve(a.size() + 1);
    std::uint64_t carry = 0;
    for (std::uint32_t limb : a) {
        std::uint64_t cur = static_cast<std::uint64_t>(limb) * factor + carry;
        r.push_back(static_cast<std::uint32_t>(cur % CSEC_BIGINT_BASE));
        carry = cur / CSEC_BIGINT_BASE;
    }
    if (carry) r.push_back(static_cast<std::uint32_t>(carry));
    return r;
}

// Schoolbook long division of magnitudes (b non-empty): quotient and remainder, base 1e9, one
// quotient limb at a time found by binary search.
void csecMagDivMod(const std::vector<std::uint32_t>& a, const std::vector<std::uint32_t>& b,
                   std::vector<std::uint32_t>& quotient, std::vector<std::uint32_t>& remainder) {
    quotient.assign(a.size(), 0);
    remainder.clear();
    for (std::size_t i = a.size(); i-- > 0;) {
        remainder.insert(remainder.begin(), a[i]);   // remainder = remainder * base + a[i]
        csecBigIntTrim(remainder);
        std::uint32_t lo = 0, hi = CSEC_BIGINT_BASE - 1;
        while (lo < hi) {
            std::uint32_t mid = lo + (hi - lo + 1) / 2;
            if (csecMagCmp(csecMagMulSmall(b, mid), remainder) <= 0) lo = mid; else hi = mid - 1;
        }
        quotient[i] = lo;
        if (lo) remainder = csecMagSub(remainder, csecMagMulSmall(b, lo));
    }
    csecBigIntTrim(quotient);
    csecBigIntTrim(remainder);
}

}  // namespace

extern "C" {

void* csec_bigint_from_i64(long long value) {
    int sign = value > 0 ? 1 : (value < 0 ? -1 : 0);
    unsigned long long u = value < 0
        ? (0ULL - static_cast<unsigned long long>(value))   // two's-complement-safe abs
        : static_cast<unsigned long long>(value);
    std::vector<std::uint32_t> mag;
    while (u) { mag.push_back(static_cast<std::uint32_t>(u % CSEC_BIGINT_BASE)); u /= CSEC_BIGINT_BASE; }
    return csecBigIntAlloc(sign, std::move(mag));
}

// Parse an arbitrarily long decimal string (optional leading +/-) into a Nat handle. This is how a
// natural-number literal too large for 64 bits reaches the program exactly.
void* csec_bigint_from_decimal(const char* text) {
    if (!text) return csecBigIntAlloc(0, {});
    const char* p = text;
    int sign = 1;
    if (*p == '+' || *p == '-') { if (*p == '-') sign = -1; ++p; }
    std::vector<std::uint32_t> mag;   // little-endian base 1e9
    for (; *p; ++p) {
        if (*p < '0' || *p > '9') continue;              // ignore stray separators
        std::uint64_t carry = static_cast<std::uint64_t>(*p - '0');
        for (std::size_t i = 0; i < mag.size(); ++i) {   // mag = mag*10 + digit
            std::uint64_t cur = static_cast<std::uint64_t>(mag[i]) * 10u + carry;
            mag[i] = static_cast<std::uint32_t>(cur % CSEC_BIGINT_BASE);
            carry = cur / CSEC_BIGINT_BASE;
        }
        while (carry) { mag.push_back(static_cast<std::uint32_t>(carry % CSEC_BIGINT_BASE)); carry /= CSEC_BIGINT_BASE; }
    }
    return csecBigIntAlloc(sign, std::move(mag));
}

static CsecBigInt* csecBigIntOrZero(void* handle) {
    static CsecBigInt zero;
    return handle ? static_cast<CsecBigInt*>(handle) : &zero;
}

void* csec_bigint_add(void* a, void* b) {
    return csecBigIntAddSigned(csecBigIntOrZero(a), csecBigIntOrZero(b));
}

void* csec_bigint_sub(void* a, void* b) {
    auto* bb = csecBigIntOrZero(b);
    CsecBigInt negated;                 // -b, without allocating
    negated.sign = -bb->sign;
    negated.mag = bb->mag;
    return csecBigIntAddSigned(csecBigIntOrZero(a), &negated);
}

void* csec_bigint_mul(void* a, void* b) {
    auto* aa = csecBigIntOrZero(a);
    auto* bb = csecBigIntOrZero(b);
    if (aa->sign == 0 || bb->sign == 0) return csecBigIntAlloc(0, {});
    return csecBigIntAlloc(aa->sign * bb->sign, csecMagMul(aa->mag, bb->mag));
}

int csec_bigint_cmp(void* a, void* b) {
    auto* aa = csecBigIntOrZero(a);
    auto* bb = csecBigIntOrZero(b);
    if (aa->sign != bb->sign) return aa->sign < bb->sign ? -1 : 1;
    if (aa->sign == 0) return 0;
    int c = csecMagCmp(aa->mag, bb->mag);
    return aa->sign > 0 ? c : -c;       // for negatives, larger magnitude is smaller value
}

char* csec_bigint_to_string(void* a) {
    auto* v = static_cast<CsecBigInt*>(a);
    if (!v || v->sign == 0) {
        char* z = static_cast<char*>(std::malloc(2));
        if (!z) return nullptr;
        z[0] = '0'; z[1] = '\0';
        return z;
    }
    std::string s;
    if (v->sign < 0) s.push_back('-');
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%u", v->mag.back());   // top limb: no zero padding
    s += buf;
    for (std::size_t i = v->mag.size() - 1; i-- > 0;) {
        std::snprintf(buf, sizeof(buf), "%09u", v->mag[i]); // lower limbs: 9-digit padded
        s += buf;
    }
    char* result = static_cast<char*>(std::malloc(s.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, s.c_str(), s.size() + 1);
    return result;
}

// Truncating division (sign of the quotient = product of signs); division by zero yields 0.
void* csec_bigint_div(void* a, void* b) {
    auto* aa = static_cast<CsecBigInt*>(a);
    auto* bb = static_cast<CsecBigInt*>(b);
    if (!aa || !bb || bb->sign == 0 || aa->sign == 0) return csecBigIntAlloc(0, {});
    std::vector<std::uint32_t> q, r;
    csecMagDivMod(aa->mag, bb->mag, q, r);
    return csecBigIntAlloc(aa->sign * bb->sign, std::move(q));
}

// Remainder with the sign of the dividend (like C's %); modulo zero yields 0.
void* csec_bigint_mod(void* a, void* b) {
    auto* aa = static_cast<CsecBigInt*>(a);
    auto* bb = static_cast<CsecBigInt*>(b);
    if (!aa || !bb || bb->sign == 0 || aa->sign == 0) return csecBigIntAlloc(0, {});
    std::vector<std::uint32_t> q, r;
    csecMagDivMod(aa->mag, bb->mag, q, r);
    return csecBigIntAlloc(aa->sign, std::move(r));
}

void* csec_bigint_pow(void* a, long long exponent) {
    auto* base = static_cast<CsecBigInt*>(a);
    if (!base || exponent < 0) return csecBigIntAlloc(0, {});
    std::vector<std::uint32_t> result{ 1 };
    std::vector<std::uint32_t> square = base->mag;
    int sign = (base->sign < 0 && (exponent & 1)) ? -1 : 1;
    if (base->sign == 0) return csecBigIntAlloc(exponent == 0 ? 1 : 0, exponent == 0 ? std::vector<std::uint32_t>{ 1 } : std::vector<std::uint32_t>{});
    while (exponent > 0) {
        if (exponent & 1) result = csecMagMul(result, square);
        exponent >>= 1;
        if (exponent) square = csecMagMul(square, square);
    }
    return csecBigIntAlloc(sign, std::move(result));
}

void* csec_bigint_gcd(void* a, void* b) {
    auto* aa = static_cast<CsecBigInt*>(a);
    auto* bb = static_cast<CsecBigInt*>(b);
    std::vector<std::uint32_t> x = aa ? aa->mag : std::vector<std::uint32_t>{};
    std::vector<std::uint32_t> y = bb ? bb->mag : std::vector<std::uint32_t>{};
    while (!y.empty()) {
        std::vector<std::uint32_t> q, r;
        csecMagDivMod(x, y, q, r);
        x = std::move(y);
        y = std::move(r);
    }
    return csecBigIntAlloc(1, std::move(x));
}

// Floor of the square root of a non-negative value (Newton iteration); negatives yield 0.
void* csec_bigint_isqrt(void* a) {
    auto* aa = static_cast<CsecBigInt*>(a);
    if (!aa || aa->sign <= 0) return csecBigIntAlloc(0, {});
    // Start above the root: 10^(ceil(digits/2)) in base-1e9 limbs.
    std::vector<std::uint32_t> x(aa->mag.size() / 2 + 1, 0);
    x.back() = 1;
    x.push_back(0);
    x.back() = 1;
    csecBigIntTrim(x);
    for (;;) {
        std::vector<std::uint32_t> q, r;
        csecMagDivMod(aa->mag, x, q, r);
        std::vector<std::uint32_t> sum = csecMagAdd(x, q);
        // y = (x + n/x) / 2
        std::vector<std::uint32_t> y, rem;
        csecMagDivMod(sum, std::vector<std::uint32_t>{ 2 }, y, rem);
        if (csecMagCmp(y, x) >= 0) break;
        x = std::move(y);
    }
    return csecBigIntAlloc(1, std::move(x));
}

long long csec_bigint_to_i64(void* a) {
    auto* aa = static_cast<CsecBigInt*>(a);
    if (!aa || aa->sign == 0) return 0;
    unsigned long long value = 0;
    for (std::size_t i = aa->mag.size(); i-- > 0;) value = value * CSEC_BIGINT_BASE + aa->mag[i];
    return aa->sign < 0 ? -static_cast<long long>(value) : static_cast<long long>(value);
}

double csec_bigint_to_double(void* a) {
    auto* aa = static_cast<CsecBigInt*>(a);
    if (!aa || aa->sign == 0) return 0.0;
    double value = 0.0;
    for (std::size_t i = aa->mag.size(); i-- > 0;) value = value * static_cast<double>(CSEC_BIGINT_BASE) + aa->mag[i];
    return aa->sign < 0 ? -value : value;
}

int csec_bigint_digit_count(void* a) {
    char* text = csec_bigint_to_string(a);
    if (!text) return 0;
    int digits = 0;
    for (const char* p = text; *p; ++p) if (*p >= '0' && *p <= '9') ++digits;
    std::free(text);
    return digits;
}

// Deterministic Miller-Rabin for values below 3.3e24 with the first 13 prime bases; for larger values
// the answer is "probably prime" with error < 4^-13.
int csec_bigint_is_prime(void* a) {
    auto* n = static_cast<CsecBigInt*>(a);
    if (!n || n->sign <= 0) return 0;
    static const std::uint32_t smallPrimes[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41 };
    for (std::uint32_t p : smallPrimes) {
        std::vector<std::uint32_t> pm{ p };
        int c = csecMagCmp(n->mag, pm);
        if (c == 0) return 1;
        if (c < 0) return 0;
        std::vector<std::uint32_t> q, r;
        csecMagDivMod(n->mag, pm, q, r);
        if (r.empty()) return 0;
    }
    const std::vector<std::uint32_t> one{ 1 };
    const std::vector<std::uint32_t> nMinus1 = csecMagSub(n->mag, one);
    std::vector<std::uint32_t> d = nMinus1;
    int s = 0;
    for (;;) {
        std::vector<std::uint32_t> q, r;
        csecMagDivMod(d, std::vector<std::uint32_t>{ 2 }, q, r);
        if (!r.empty()) break;
        d = std::move(q);
        ++s;
    }
    auto mulMod = [&](const std::vector<std::uint32_t>& x, const std::vector<std::uint32_t>& y) {
        std::vector<std::uint32_t> q, r;
        csecMagDivMod(csecMagMul(x, y), n->mag, q, r);
        return r;
    };
    for (std::uint32_t base : smallPrimes) {
        // x = base^d mod n
        std::vector<std::uint32_t> x{ 1 }, b{ base }, e = d;
        while (!e.empty()) {
            std::vector<std::uint32_t> q, r;
            csecMagDivMod(e, std::vector<std::uint32_t>{ 2 }, q, r);
            if (!r.empty()) x = mulMod(x, b);
            b = mulMod(b, b);
            e = std::move(q);
        }
        if (csecMagCmp(x, one) == 0 || csecMagCmp(x, nMinus1) == 0) continue;
        bool composite = true;
        for (int i = 1; i < s; ++i) {
            x = mulMod(x, x);
            if (csecMagCmp(x, nMinus1) == 0) { composite = false; break; }
        }
        if (composite) return 0;
    }
    return 1;
}

}  // extern "C"
