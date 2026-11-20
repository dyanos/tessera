// ============================================================================
//  tensor_runtime.inc  --  tensor operations callable from the self-host compiler
//
//  tess lowers tensor arithmetic as inline IR (TensorRuntime.h). The self-host
//  compiler emits IR by string-building and cannot practically inline those
//  loops, so it calls these runtime helpers instead. The struct layout matches
//  tess's  %csec.tensor { i64 rank, i64* dims, i64 count, double* data }  so a
//  handle is interchangeable. Handles are heap objects, caller-owned (leaked),
//  like the string and bigint helpers.
// ============================================================================

namespace {
struct CsecTensor {
    long long rank;
    long long* dims;
    long long count;
    double* data;
};

[[noreturn]] void csecTensorOutOfMemory(long long count) {
    std::fflush(stdout);
    std::fprintf(stderr, "MemoryError: cannot allocate a tensor of %lld elements\n", count);
    std::fflush(stderr);
    std::exit(1);
}

CsecTensor* csecTensorAlloc(long long rank, long long count) {
    const size_t maxElements = (std::numeric_limits<size_t>::max)() / sizeof(double);
    if (count < 0 || static_cast<unsigned long long>(count) > maxElements ||
        rank < 0 || static_cast<unsigned long long>(rank) > maxElements) {
        csecTensorOutOfMemory(count);
    }
    auto* t = static_cast<CsecTensor*>(csecAlloc(sizeof(CsecTensor)));
    if (!t) csecTensorOutOfMemory(count);
    t->rank = rank;
    t->count = count;
    t->dims = static_cast<long long*>(csecAlloc(sizeof(long long) * static_cast<size_t>(rank > 0 ? rank : 1)));
    t->data = static_cast<double*>(csecAlloc(sizeof(double) * static_cast<size_t>(count > 0 ? count : 1)));
    if (!t->dims || !t->data) csecTensorOutOfMemory(count);
    return t;
}

CsecTensor* csecTensorLike(const CsecTensor* src) {
    CsecTensor* r = csecTensorAlloc(src->rank, src->count);
    for (long long i = 0; i < src->rank; ++i) r->dims[i] = src->dims[i];
    return r;
}
}  // namespace

extern "C" {

// Create a flat tensor of `count` elements, every element initialised to `fill`. For a rank-1 vector
// `count` is the length; a higher-rank tensor is stored flat with `count` = product of its dimensions.
void* csec_tensor_new(long long count, double fill) {
    if (count < 0) count = 0;
    CsecTensor* t = csecTensorAlloc(1, count);
    t->dims[0] = count;
    for (long long i = 0; i < count; ++i) t->data[i] = fill;
    return t;
}

double csec_tensor_get(void* tp, long long i) {
    auto* t = static_cast<CsecTensor*>(tp);
    if (!t || i < 0 || i >= t->count) return 0.0;
    return t->data[i];
}

// Rank-2 (matrix) tensor: `rows x cols`, stored flat row-major.
void* csec_tensor_new2(long long rows, long long cols, double fill) {
    if (rows < 0) rows = 0;
    if (cols < 0) cols = 0;
    if (cols > 0 && rows > (std::numeric_limits<long long>::max)() / cols) csecTensorOutOfMemory(-1);
    long long count = rows * cols;
    CsecTensor* t = csecTensorAlloc(2, count);
    if (t->rank >= 2) { t->dims[0] = rows; t->dims[1] = cols; }
    for (long long i = 0; i < count; ++i) t->data[i] = fill;
    return t;
}

double csec_tensor_get2(void* tp, long long i, long long j) {
    auto* t = static_cast<CsecTensor*>(tp);
    if (!t) return 0.0;
    long long cols = t->rank >= 2 ? t->dims[1] : 1;
    long long idx = i * cols + j;
    if (idx < 0 || idx >= t->count) return 0.0;
    return t->data[idx];
}

void csec_tensor_set2(void* tp, long long i, long long j, double v) {
    auto* t = static_cast<CsecTensor*>(tp);
    if (!t) return;
    long long cols = t->rank >= 2 ? t->dims[1] : 1;
    long long idx = i * cols + j;
    if (idx < 0 || idx >= t->count) return;
    t->data[idx] = v;
}

void csec_tensor_set(void* tp, long long i, double v) {
    auto* t = static_cast<CsecTensor*>(tp);
    if (!t || i < 0 || i >= t->count) return;
    t->data[i] = v;
}

void* csec_tensor_add(void* ap, void* bp) {
    auto* a = static_cast<CsecTensor*>(ap);
    auto* b = static_cast<CsecTensor*>(bp);
    CsecTensor* r = csecTensorLike(a);
    for (long long i = 0; i < a->count; ++i) r->data[i] = a->data[i] + (i < b->count ? b->data[i] : 0.0);
    return r;
}

void* csec_tensor_sub(void* ap, void* bp) {
    auto* a = static_cast<CsecTensor*>(ap);
    auto* b = static_cast<CsecTensor*>(bp);
    CsecTensor* r = csecTensorLike(a);
    for (long long i = 0; i < a->count; ++i) r->data[i] = a->data[i] - (i < b->count ? b->data[i] : 0.0);
    return r;
}

// Element-wise (Hadamard) product of two equal-shaped tensors.
void* csec_tensor_mul(void* ap, void* bp) {
    auto* a = static_cast<CsecTensor*>(ap);
    auto* b = static_cast<CsecTensor*>(bp);
    CsecTensor* r = csecTensorLike(a);
    for (long long i = 0; i < a->count; ++i) r->data[i] = a->data[i] * (i < b->count ? b->data[i] : 0.0);
    return r;
}

void* csec_tensor_scale(void* tp, double s) {
    auto* t = static_cast<CsecTensor*>(tp);
    CsecTensor* r = csecTensorLike(t);
    for (long long i = 0; i < t->count; ++i) r->data[i] = t->data[i] * s;
    return r;
}

void* csec_tensor_div_scalar(void* tp, double s) {
    auto* t = static_cast<CsecTensor*>(tp);
    CsecTensor* r = csecTensorLike(t);
    for (long long i = 0; i < t->count; ++i) r->data[i] = t->data[i] / s;
    return r;
}

// Dot product (sum of element-wise products) of two equal-length tensors.
double csec_tensor_inner(void* ap, void* bp) {
    auto* a = static_cast<CsecTensor*>(ap);
    auto* b = static_cast<CsecTensor*>(bp);
    double sum = 0.0;
    long long n = a->count < b->count ? a->count : b->count;
    for (long long i = 0; i < n; ++i) sum += a->data[i] * b->data[i];
    return sum;
}

// ---- reductions -----------------------------------------------------------------------------
// op: 0 sum, 1 mean, 2 max, 3 min, 4 product.
static double csecReduceRange(const double* data, long long count, long long stride, int op) {
    if (count <= 0) return op == 4 ? 1.0 : 0.0;
    double acc = op == 4 ? 1.0 : (op == 2 || op == 3 ? data[0] : 0.0);
    for (long long i = 0; i < count; ++i) {
        const double v = data[i * stride];
        switch (op) {
            case 2: if (v > acc) acc = v; break;
            case 3: if (v < acc) acc = v; break;
            case 4: acc *= v; break;
            default: acc += v; break;
        }
    }
    return op == 1 ? acc / static_cast<double>(count) : acc;
}

// Reduction of every element.
double csec_tensor_reduce_all(void* tp, int op) {
    auto* t = static_cast<CsecTensor*>(tp);
    return csecReduceRange(t->data, t->count, 1, op);
}

// Reduction along one axis: the result has that axis removed (a rank-1 tensor gives a rank-1
// tensor of one element). A negative axis counts from the last.
void* csec_tensor_reduce_axis(void* tp, long long axis, int op) {
    auto* t = static_cast<CsecTensor*>(tp);
    if (axis < 0) axis += t->rank;
    if (axis < 0 || axis >= t->rank) {
        std::fprintf(stderr, "TensorError: axis %lld is out of range for a rank-%lld tensor\n", axis, t->rank);
        std::exit(1);
    }
    long long outer = 1, inner = 1;
    for (long long i = 0; i < axis; ++i) outer *= t->dims[i];
    for (long long i = axis + 1; i < t->rank; ++i) inner *= t->dims[i];
    const long long n = t->dims[axis];
    const long long rank = t->rank > 1 ? t->rank - 1 : 1;
    CsecTensor* r = csecTensorAlloc(rank, outer * inner);
    if (t->rank > 1) {
        long long k = 0;
        for (long long i = 0; i < t->rank; ++i) if (i != axis) r->dims[k++] = t->dims[i];
    }
    else {
        r->dims[0] = 1;
    }
    for (long long o = 0; o < outer; ++o) {
        for (long long in = 0; in < inner; ++in) {
            r->data[o * inner + in] = csecReduceRange(t->data + o * n * inner + in, n, inner, op);
        }
    }
    return r;
}

// Flat (row-major) index of the largest (wantMax != 0) or smallest element; -1 for an empty tensor.
long long csec_tensor_argext(void* tp, int wantMax) {
    auto* t = static_cast<CsecTensor*>(tp);
    if (t->count <= 0) return -1;
    long long best = 0;
    for (long long i = 1; i < t->count; ++i) {
        if (wantMax ? t->data[i] > t->data[best] : t->data[i] < t->data[best]) best = i;
    }
    return best;
}

// A tensor with the shape of `t`, every element `value`.
void* csec_tensor_full_like(void* tp, double value) {
    auto* t = static_cast<CsecTensor*>(tp);
    CsecTensor* r = csecTensorLike(t);
    for (long long i = 0; i < r->count; ++i) r->data[i] = value;
    return r;
}

// ---- random numbers (xoshiro256**) ----------------------------------------------------------
struct CsecRng { unsigned long long s[4]; bool hasSpare; double spare; };

static unsigned long long csecSplitMix(unsigned long long& x) {
    unsigned long long z = (x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static void csecRngSeed(CsecRng* r, unsigned long long seed) {
    for (auto& word : r->s) word = csecSplitMix(seed);
    r->hasSpare = false;
    r->spare = 0.0;
}

static CsecRng* csecGlobalRng() {
    static CsecRng rng;
    static bool seeded = false;
    if (!seeded) {
        csecRngSeed(&rng, static_cast<unsigned long long>(std::time(nullptr)) ^ 0x5DEECE66DULL);
        seeded = true;
    }
    return &rng;
}

static CsecRng* csecRng(long long handle) {
    return handle ? reinterpret_cast<CsecRng*>(handle) : csecGlobalRng();
}

static unsigned long long csecRotl(unsigned long long x, int k) { return (x << k) | (x >> (64 - k)); }

static unsigned long long csecRngNext(CsecRng* r) {
    const unsigned long long result = csecRotl(r->s[1] * 5, 7) * 9;
    const unsigned long long t = r->s[1] << 17;
    r->s[2] ^= r->s[0];
    r->s[3] ^= r->s[1];
    r->s[1] ^= r->s[2];
    r->s[0] ^= r->s[3];
    r->s[2] ^= t;
    r->s[3] = csecRotl(r->s[3], 45);
    return result;
}

// A generator with its own state (0 is the shared global generator).
long long csec_rng_new(long long seed) {
    auto* r = static_cast<CsecRng*>(csecAlloc(sizeof(CsecRng)));
    csecRngSeed(r, static_cast<unsigned long long>(seed));
    return reinterpret_cast<long long>(r);
}

void csec_rng_free(long long handle) {
    if (handle) csecFree(reinterpret_cast<void*>(handle));
}

void csec_rng_seed(long long handle, long long seed) {
    csecRngSeed(csecRng(handle), static_cast<unsigned long long>(seed));
}

// Uniform in [0, 1).
double csec_rng_uniform(long long handle) {
    return static_cast<double>(csecRngNext(csecRng(handle)) >> 11) * (1.0 / 9007199254740992.0);
}

// Uniform integer in [low, high).
long long csec_rng_int(long long handle, long long low, long long high) {
    if (high <= low) return low;
    const unsigned long long span = static_cast<unsigned long long>(high - low);
    const unsigned long long limit = ~0ULL - (~0ULL % span);
    unsigned long long x;
    do { x = csecRngNext(csecRng(handle)); } while (x >= limit);
    return low + static_cast<long long>(x % span);
}

// Normal (Gaussian) with the given mean and standard deviation (Marsaglia polar method).
double csec_rng_normal(long long handle, double mean, double sd) {
    CsecRng* r = csecRng(handle);
    if (r->hasSpare) {
        r->hasSpare = false;
        return mean + sd * r->spare;
    }
    double u, v, q;
    do {
        u = 2.0 * csec_rng_uniform(handle) - 1.0;
        v = 2.0 * csec_rng_uniform(handle) - 1.0;
        q = u * u + v * v;
    } while (q >= 1.0 || q == 0.0);
    const double f = std::sqrt(-2.0 * std::log(q) / q);
    r->spare = v * f;
    r->hasSpare = true;
    return mean + sd * u * f;
}

// Fills `t` with random numbers: dist 0 uniform in [a, b), 1 normal with mean a and deviation b.
void csec_tensor_fill_random(void* tp, long long handle, int dist, double a, double b) {
    auto* t = static_cast<CsecTensor*>(tp);
    for (long long i = 0; i < t->count; ++i) {
        t->data[i] = dist == 1 ? csec_rng_normal(handle, a, b) : a + (b - a) * csec_rng_uniform(handle);
    }
}

}  // extern "C"
