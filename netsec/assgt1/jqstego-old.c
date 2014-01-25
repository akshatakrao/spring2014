/* JPEG Quantization error Steganography - 30.01.2012 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/resource.h>

#include <netpbm/pam.h>
#include "jpeglib.h"
#include <gcrypt.h>
#include <lzma.h>
#include <ccgi.h>

/* number of coefficients in a permutation block */
#define JQST_DEFAULT_PBLKSIZE   (DCTSIZE)
/* number of permuted coefficients used for deriving key */
#define JQST_INITIAL_COEF_KDF   (DCTSIZE2)
/* minimal allowed embedding block size */
#define JQST_MIN_BLK_SIZE       (DCTSIZE2/2)
/* maximal absolute coefficient value for statistics */
#define JQST_MAX_COEF_STAT      (127)
/* minimal payload size in bytes to be extracted */
#define JQST_MIN_PAYLOAD        (32)
/* encoded message is always a multiple in bytes of this */
#define JQST_MSG_SIZE_DELTA     (4)
/* memory limit for decompression in bytes */
#define JQST_MAX_DECOMP_MEM     (32*1024*1024)
/* prefix of temporary cgi upload files */
#define JQST_TMP_PREFIX         "/tmp/jqstego-cgi-"
/* maximal message length for embedding/extracting under cgi */
#define JQST_MAX_CGI_MSG_LEN    (160)
/* cpu time limit in milliseconds(?!) for cgi mode */
#define JQST_CGI_LIMIT_CPU      (120*1000)
/* file size limit in bytes for cgi mode */
#define JQST_CGI_LIMIT_FSIZE    (20*1024*1024)

#define MAX(_a, _b)         ((_a) > (_b)? (_a):(_b))
#define ARRAY_SIZE(_a)      (sizeof(_a)/sizeof((_a)[0]))
#define ROUND_UP(_a, _b)    ((_a) + ((_b) - (_a)%(_b)))
#define ROUND_DOWN(_a, _b)  ((_a) - ((_a)%(_b)))

#if (0 != (DCTSIZE2 % JQST_DEFAULT_PBLKSIZE))
# error "The permutation block size must be a divisor of the DCT block size!"
#endif

#if (0 != (JQST_MIN_PAYLOAD % JQST_MSG_SIZE_DELTA))
# error "The minimum message size must be a multiple of the delta size!"
#endif

/* Common data structures */
enum jqst_html_tag {
    JQST_TAG_TEXT = 0,
    JQST_TAG_HTML,
    JQST_TAG_HEAD,
    JQST_TAG_BODY,
    JQST_TAG_TITLE,
    JQST_TAG_META,
    JQST_TAG_LINK,
    JQST_TAG_FORM,
    JQST_TAG_FIELDSET,
    JQST_TAG_LEGEND,
    JQST_TAG_TABLE,
    JQST_TAG_TR,
    JQST_TAG_TD,
    JQST_TAG_LABEL,
    JQST_TAG_TEXTAREA,
    JQST_TAG_INPUT,
    JQST_TAG_SELECT,
    JQST_TAG_OPTION
};

static const char *jqst_tag_name[] = {
    NULL, "html", "head", "body", "title", "meta", "link", "form", "fieldset",
    "legend", "table", "tr", "td", "label", "textarea", "input", "select",
    "option"
};

enum jqst_attrib_type {
    JQST_TYPE_TEXT,
    JQST_TYPE_NUMBER
};

enum jqst_tag_attrib {
    JQST_ATR_NULL = 0,
    JQST_ATR_HTTP_EQUIV,
    JQST_ATR_CONTENT,
    JQST_ATR_REL,
    JQST_ATR_TYPE,
    JQST_ATR_HREF,
    JQST_ATR_METHOD,
    JQST_ATR_ENCTYPE,
    JQST_ATR_ACTION,
    JQST_ATR_CELLPADDING,
    JQST_ATR_FOR,
    JQST_ATR_COLSPAN,
    JQST_ATR_NAME,
    JQST_ATR_ID,
    JQST_ATR_ROWS,
    JQST_ATR_COLS,
    JQST_ATR_SIZE,
    JQST_ATR_VALUE,
    JQST_ATR_CHECKED,
    JQST_ATR_ALIGN
};

static const char *jqst_tag_attrib[] = {
    NULL, "http-equiv", "content", "rel", "type", "href", "method", "enctype",
    "action", "cellpadding", "for", "colspan", "name", "id", "rows", "cols",
    "size", "value", "checked", "align"
};

struct jqst_html_attrib {
    enum jqst_tag_attrib attrib;
    enum jqst_attrib_type type;
    union {
        const char *text;
        int number;
    } value;
};

struct jqst_html_node {
    enum jqst_html_tag tag;
    union {
        struct jqst_html_node *tag;
        const char *text;
    } contents;
    struct jqst_html_node *next;
    struct jqst_html_attrib attrib[1];
};

enum jqst_cell_type {
    JQST_TBL_END,
    JQST_TBL_NEWROW,
    JQST_TBL_INPUT,
    JQST_TBL_TEXTAREA,
    JQST_TBL_SELECT
};

enum jqst_align {
    JQST_ALIGN_NONE,
    JQST_ALIGN_RIGHT,
    JQST_ALIGN_LEFT,
    JQST_ALIGN_CENTER,
    JQST_ALIGN_BOTTOM
};

enum jqst_input_type {
    JQST_ITYPE_FILE,
    JQST_ITYPE_PASSWORD,
    JQST_ITYPE_TEXT,
    JQST_ITYPE_CHECKBOX,
    JQST_ITYPE_RADIO,
    JQST_ITYPE_SUBMIT,
    JQST_ITYPE_RESET
};

struct jqst_html_input {
    enum jqst_input_type type;
    int checked;
};

struct jqst_html_textarea {
    unsigned int cols, rows;
};

struct jqst_html_select {
    const char **opt;
};

struct jqst_html_formtable_cell {
    enum jqst_cell_type type;
    unsigned int span;
    enum jqst_align align, label_align;
    const char *name, *id, *value, *label;
    unsigned int size, label_span;
    int label_behind;
    union {
        struct jqst_html_input input;
        struct jqst_html_textarea textarea;
        struct jqst_html_select select;
    } tspec;
};

struct jqst_html_formtable {
    unsigned int pad;
    const struct jqst_html_formtable_cell *cell;
};

struct jqst_options {
    boolean arith;
    boolean optimize;
    int smooth;
    J_DCT_METHOD dct;
    int quality;
};

struct jqst_message {
    unsigned int len, size;
    uint8_t *data;
};

struct jqst_prng_state {
    uint8_t state[16];
    uint8_t ctr[16];
    uint8_t out[16];
    unsigned int oindex;
    gcry_cipher_hd_t cipher;
};

struct jqst_permutation {
    uint32_t pblksize, plen;
    uint32_t *index;
};

struct jqst_jpeg {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    /* output file */
    FILE *fout;
    /* source image sample line data */
    JSAMPROW scanline;
    sample maxval;
    /* coefficients buffer per component */
    JCOEF *coef[MAX_COMPONENTS];
    /* quantization errors per component */
    signed char *querr[MAX_COMPONENTS];
    /* quantization divisors for one block per component */
    double qudiv[MAX_COMPONENTS][DCTSIZE2];
    double qudiv_min[MAX_COMPONENTS];
    /* maximum threshold at given block size estimation */
    unsigned int blksize;
    unsigned char querr_minchg[MAX_COMPONENTS];
    /* absolute quantization error histogram */
    uint32_t querr_hist[MAX_COMPONENTS][(UINT8_MAX + 1)/2 + 1];
    uint32_t querr_count[MAX_COMPONENTS];
    uint32_t msgbits;
    /* coefficent change histogram */
    uint32_t coef_count[MAX_COMPONENTS][DCTSIZE2];
    int coef_hist[MAX_COMPONENTS][DCTSIZE2][2*JQST_MAX_COEF_STAT + 1];
    /* block permutations */
    struct jqst_permutation perm[MAX_COMPONENTS];
    /* use absolute quentization error instead of relative */
    int optimize_absolute_querr;
};

/* HTML form data for CGI */
#define JQST_DCTSTR_ISLOW "slow integer"
#define JQST_DCTSTR_IFAST "fast integer"
#define JQST_DCTSTR_FLOAT "floating point"
static const char *jqst_dct_methods[] = {
    JQST_DCTSTR_ISLOW, JQST_DCTSTR_IFAST, JQST_DCTSTR_FLOAT, NULL
};
static const struct jqst_html_formtable_cell jqst_formcells_extract[] = {
    {.type = JQST_TBL_INPUT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "imgfile", .id = "imgfile", .value = NULL,
        .label_span = 1, .label_behind = 0, .label_align = JQST_ALIGN_NONE,
        .label = "JPEG stego image file:", .size = 30, .tspec = {.input = {.type = JQST_ITYPE_FILE}}
    },
    {.type = JQST_TBL_INPUT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "passphr", .id = "passphr", .value = NULL,
        .label_span = 1, .label_behind = 0, .label_align = JQST_ALIGN_NONE,
        .label = "Passphrase:", .size = 30, .tspec = {.input = {.type = JQST_ITYPE_PASSWORD}}
    },
    {.type = JQST_TBL_NEWROW},
    {.type = JQST_TBL_INPUT, .span = 2, .align = JQST_ALIGN_CENTER,
        .name = "submit", .id = "submit", .value = "Submit",
        .label = NULL, .size = 0, .tspec = {.input = {.type = JQST_ITYPE_SUBMIT}}
    },
    {.type = JQST_TBL_INPUT, .span = 2, .align = JQST_ALIGN_CENTER,
        .name = "reset", .id = "reset", .value = "Reset",
        .label = NULL, .size = 0, .tspec = {.input = {.type = JQST_ITYPE_RESET}}
    },
    {.type = JQST_TBL_END}
}, jqst_formcells_embed[] = {
    {.type = JQST_TBL_TEXTAREA, .span = 3, .align = JQST_ALIGN_NONE,
        .name = "message", .id = "message", .value = "",
        .label_span = 1, .label_behind = 0, .label_align = JQST_ALIGN_NONE,
        .label = "Secret message:", .size = 0, .tspec = {.textarea = {.rows = 4, .cols = 72}}
    },
    {.type = JQST_TBL_NEWROW},
    {.type = JQST_TBL_INPUT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "imgfile", .id = "imgfile", .value = NULL,
        .label_span = 1, .label_behind = 0, .label_align = JQST_ALIGN_NONE,
        .label = "PNM (NetPBM) cover image file:", .size = 30, .tspec = {.input = {.type = JQST_ITYPE_FILE}}
    },
    {.type = JQST_TBL_INPUT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "passphr", .id = "passphr", .value = NULL,
        .label_span = 1, .label_behind = 0, .label_align = JQST_ALIGN_NONE,
        .label = "Passphrase:", .size = 30, .tspec = {.input = {.type = JQST_ITYPE_PASSWORD}}
    },
    {.type = JQST_TBL_NEWROW},
    {.type = JQST_TBL_INPUT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "quality", .id = "quality", .value = "85",
        .label_span = 1, .label_behind = 0, .label_align = JQST_ALIGN_NONE,
        .label = "Quality (0-100):", .size = 5, .tspec = {.input = {.type = JQST_ITYPE_TEXT}}
    },
    {.type = JQST_TBL_INPUT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "smooth", .id = "smooth", .value = "0",
        .label_span = 1, .label_behind = 0, .label_align = JQST_ALIGN_NONE,
        .label = "Smoothing (0-100):", .size = 5, .tspec = {.input = {.type = JQST_ITYPE_TEXT}}
    },
    {.type = JQST_TBL_NEWROW},
    {.type = JQST_TBL_SELECT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "dct_method", .id = "dct_method", .value = NULL,
        .label_span = 1, .label_behind = 0, .label_align = JQST_ALIGN_NONE,
        .label = "DCT method:", .size = 1, .tspec = {.select = {.opt = jqst_dct_methods}}
    },
    {.type = JQST_TBL_INPUT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "options", .id = "arith", .value = "arith",
        .label_span = 0, .label_behind = 1, .label_align = JQST_ALIGN_NONE,
        .label = "Arithmetic Coding", .size = 0, .tspec = {.input = {.type = JQST_ITYPE_CHECKBOX, .checked = 0}}
    },
    {.type = JQST_TBL_INPUT, .span = 1, .align = JQST_ALIGN_NONE,
        .name = "options", .id = "optimize", .value = "optimize",
        .label_span = 0, .label_behind = 1, .label_align = JQST_ALIGN_NONE,
        .label = "Optimized Coding", .size = 0, .tspec = {.input = {.type = JQST_ITYPE_CHECKBOX, .checked = 1}}
    },
    {.type = JQST_TBL_NEWROW},
    {.type = JQST_TBL_INPUT, .span = 2, .align = JQST_ALIGN_CENTER,
        .name = "submit", .id = "submit", .value = "Submit",
        .label = NULL, .size = 0, .tspec = {.input = {.type = JQST_ITYPE_SUBMIT}}
    },
    {.type = JQST_TBL_INPUT, .span = 2, .align = JQST_ALIGN_CENTER,
        .name = "reset", .id = "reset", .value = "Reset",
        .label = NULL, .size = 0, .tspec = {.input = {.type = JQST_ITYPE_RESET}}
    },
    {.type = JQST_TBL_END}
};


/* parse integer */
static int
jqst_parse_number(const char *str, int *num)
{
    long res;
    char *eptr = (char *)str;

    errno = 0;
    res = strtol(str, &eptr, 10);
    if (errno != 0 || str[0] == '\0' || eptr[0] != '\0' || res < INT_MIN ||
            res > INT_MAX)
        return -1;

    num[0] = res;

    return 0;
}

/* Deterministic Pseudo Random Number Generator functions */
static int
jqst_prng_init(struct jqst_prng_state *state, const uint8_t *key,
        const uint8_t *seed)
{
    gcry_error_t err;

    memset(state, 0, sizeof(state[0]));
    memcpy(state->state, seed, sizeof(state->state));
    err = gcry_cipher_open(&state->cipher, GCRY_CIPHER_AES128,
            GCRY_CIPHER_MODE_ECB, 0);
    if (0 != err)
        return -1;
    err = gcry_cipher_setkey(state->cipher, key, 16);
    if (0 != err)
        return -1;

    return 0;
}

static void
jqst_prng_destroy(struct jqst_prng_state *state)
{
    gcry_cipher_close(state->cipher);
    memset(state, 0, sizeof(state[0]));
}

static uint32_t
jqst_prng_poll(struct jqst_prng_state *state)
{
    uint32_t out;

    if (state->oindex >= ARRAY_SIZE(state->out) || state->oindex < 1) {
        uint8_t ectr[16];
        unsigned int i;
        gcry_error_t err;

        /* generate new output */
        err = gcry_cipher_encrypt(state->cipher, ectr, sizeof(ectr),
                state->ctr, sizeof(state->ctr));
        if (0 != err) {
            fprintf(stderr, "PRNG error encrypting counter\n");
            abort();
        }
        for (i = 0; i < ARRAY_SIZE(state->state); i++)
            state->state[i] ^= ectr[i];
        err = gcry_cipher_encrypt(state->cipher, state->out, sizeof(state->out),
                state->state, sizeof(state->state));
        if (0 != err) {
            fprintf(stderr, "PRNG error encrypting output\n");
            abort();
        }
        for (i = 0; i < ARRAY_SIZE(ectr); i++)
            ectr[i] ^= state->out[i];
        err = gcry_cipher_encrypt(state->cipher, state->state,
                sizeof(state->state), ectr, sizeof(ectr));
        if (0 != err) {
            fprintf(stderr, "PRNG error encrypting state\n");
            abort();
        }
        memset(ectr, 0, sizeof(ectr));
        for (i = 0; i < ARRAY_SIZE(state->ctr); i++)
            if (0 != ++(state->ctr[ARRAY_SIZE(state->ctr) - i - 1]))
                break;
        state->oindex = 0;
    }

    out  = state->out[state->oindex++] << 24;
    out += state->out[state->oindex++] << 16;
    out += state->out[state->oindex++] <<  8;
    out += state->out[state->oindex++];

    return out;
}

/* JPEG compression quantization error and coefficient callback */
static void
jqst_quant_err(int ci, JDIMENSION x, JDIMENSION y, JBLOCKROW coef,
        signed char *querr, JDIMENSION blockcnt, void *client_data)
{
    struct jqst_jpeg *jpeg = (struct jqst_jpeg *)client_data;
    const jpeg_component_info *compptr;
    unsigned int blkpos;

    /*fprintf(stderr, "ci: %d, x: %d, y: %d, %d blocks\n", ci, x, y, blockcnt);*/

    /* fill coefficient and quantization error buffers */
    compptr = &jpeg->cinfo.comp_info[ci];
    blkpos = (y*compptr->width_in_blocks + x)*DCTSIZE2;
    blockcnt *= DCTSIZE2;
    memcpy(jpeg->coef[ci] + blkpos, coef, blockcnt*sizeof(jpeg->coef[ci][0]));
    memcpy(jpeg->querr[ci] + blkpos, querr,
            blockcnt*sizeof(jpeg->querr[ci][0]));
}

static void
jqst_jpeg_destroy(struct jqst_jpeg *jpeg)
{
    int ci;

    if (NULL == jpeg)
        return;
    if (NULL != jpeg->scanline)
        free(jpeg->scanline);
    for (ci = 0; ci < jpeg->cinfo.num_components; ci++)
    {
        if (NULL != jpeg->coef[ci])
            free(jpeg->coef[ci]);
        if (NULL != jpeg->querr[ci])
            free(jpeg->querr[ci]);
        if (NULL != jpeg->perm[ci].index)
            free(jpeg->perm[ci].index);
    }
    if (NULL != jpeg->fout)
        fclose(jpeg->fout);
    jpeg_destroy_compress(&jpeg->cinfo);
    memset(jpeg, 0, sizeof(jpeg[0]));
}

static int
jqst_jpeg_init(const struct pam *image, struct jqst_jpeg *jpeg,
        struct jqst_options *opt)
{
    int ci, ret;

    if (NULL == image || NULL == jpeg)
        return -1;

    memset(jpeg, 0, sizeof(jpeg[0]));
    memset(jpeg->querr_minchg, 0xFF, sizeof(jpeg->querr_minchg));

    jpeg->maxval = image->maxval;
    jpeg->scanline = malloc(image->width*image->depth*
            sizeof(jpeg->scanline[0]));
    if (NULL == jpeg->scanline)
        return -1;

    jpeg->cinfo.err = jpeg_std_error(&jpeg->jerr);
    jpeg_create_compress(&jpeg->cinfo);

    jpeg->cinfo.image_width = image->width;
    jpeg->cinfo.image_height = image->height;
    jpeg->cinfo.input_components = image->depth;
    switch (image->depth) {
        case 1:
            jpeg->cinfo.in_color_space = JCS_GRAYSCALE;
            break;
        case 3:
            jpeg->cinfo.in_color_space = JCS_RGB;
            break;
        case 4:
            jpeg->cinfo.in_color_space = JCS_CMYK;
            break;
        default:
            jpeg->cinfo.in_color_space = JCS_UNKNOWN;
            break;
    }
    jpeg_set_defaults(&jpeg->cinfo);

    jpeg->cinfo.client_data = jpeg;
    jpeg->cinfo.stego_info = &jqst_quant_err;
    for (ci = 0; ci < MAX_COMPONENTS; ci++)
        jpeg->cinfo.quant_div[ci] = jpeg->qudiv[ci];
    if (NULL != opt) {
        jpeg->cinfo.arith_code = opt->arith;
        jpeg->cinfo.optimize_coding = opt->optimize;
        jpeg->cinfo.smoothing_factor = opt->smooth;
        jpeg->cinfo.dct_method = opt->dct;
        jpeg_set_quality(&jpeg->cinfo, opt->quality, TRUE);
    } else {
        jpeg->cinfo.dct_method = JDCT_ISLOW;
        jpeg->cinfo.optimize_coding = TRUE;
        jpeg_set_quality(&jpeg->cinfo, 85, TRUE);
    }
    jpeg->optimize_absolute_querr = 1;

    jpeg->fout = fopen("/dev/null", "wb");
    if (NULL == jpeg->fout) {
        jqst_jpeg_destroy(jpeg);
        return -1;
    }
    jpeg_stdio_dest(&jpeg->cinfo, jpeg->fout);

    jpeg_start_compress(&jpeg->cinfo, FALSE);

    /* allocate buffers */
    ret = 0;
    for (ci = 0; ci < jpeg->cinfo.num_components; ci++)
    {
        const jpeg_component_info *compptr = &jpeg->cinfo.comp_info[ci];
        size_t blocks;

        blocks = compptr->width_in_blocks*compptr->height_in_blocks;
        jpeg->coef[ci] = malloc(blocks*DCTSIZE2*sizeof(jpeg->coef[ci][0]));
        jpeg->querr[ci] = malloc(blocks*DCTSIZE2*sizeof(jpeg->querr[ci][0]));
        if (NULL == jpeg->coef[ci] || NULL == jpeg->querr[ci]) {
            ret = -1;
            break;
        }
    }
    if (ret < 0) {
        jqst_jpeg_destroy(jpeg);
    }

    return ret;
}

static int
jqst_jpeg_compute_permutations(struct jqst_permutation *perm, int num_components,
        const jpeg_component_info *comp_info, int image_width, int image_height,
        const char *pass, unsigned int pblksize)
{
    struct jqst_prng_state prng;
    uint8_t salt[8], seed[32];
    int ci, ret;
    gcry_error_t err;

    salt[0] = (image_width >> 8) & 0xFFU;
    salt[1] = (image_width) & 0xFFU;
    salt[2] = (image_height >> 8) & 0xFFU;
    salt[3] = (image_height) & 0xFFU;
    salt[4] = num_components;
    salt[5] = salt[6] = salt[7] = 0;
    err = gcry_kdf_derive(pass, strlen(pass), GCRY_KDF_ITERSALTED_S2K,
            GCRY_MD_SHA256, salt, sizeof (salt), 65536, sizeof(seed), &seed);
    if (0 != err) {
        fprintf(stderr, "deriving seed from pass phrase failed: %s\n",
                gcry_strerror(err));
        return -1;
    }

    ret = jqst_prng_init(&prng, seed, seed + ARRAY_SIZE(seed)/2);
    if (ret < 0) {
        fprintf(stderr, "seeding PRNG failed\n");
        return -1;
    }
    memset(seed, 0, sizeof(seed));
    for (ci = 0; ci < num_components; ci++)
    {
        const jpeg_component_info *compptr = &comp_info[ci];
        uint32_t i;

        perm[ci].pblksize = pblksize;
        perm[ci].plen = (compptr->width_in_blocks*
                compptr->height_in_blocks*DCTSIZE2)/pblksize;
        perm[ci].index = malloc(perm[ci].plen*sizeof(perm[ci].index[0]));
        if (NULL == perm[ci].index)
            return -1;

        for (i = 0; i < perm[ci].plen; i++)
            perm[ci].index[i] = i;
        for (i = perm[ci].plen - 1; i > 1; i--)
        {
            uint32_t j, tmp;

            do {
                j = jqst_prng_poll(&prng);
            } while (j > UINT32_MAX - 1 - UINT32_MAX%(i + 1));
            j %= (i + 1);

            tmp = perm[ci].index[i];
            perm[ci].index[i] = perm[ci].index[j];
            perm[ci].index[j] = tmp;
        }
    }
    jqst_prng_destroy(&prng);

    return 0;
}

/* Compute 256 bit key from pass phrase and iv from first JQST_INITIAL_COEF_KDF
 * coefficients in permutation of JPEG file */
static int
jqst_jpeg_getkey(int num_components, int image_width, int image_height,
        const struct jqst_permutation *perm, JCOEF **coef,
        const char *pass, uint8_t *key, uint8_t *ivec)
{
    gcry_error_t err1, err2;
    uint8_t *pblock = NULL, salt[8];
    unsigned int plen, i;

    salt[0] = (image_width >> 8) & 0xFFU;
    salt[1] = (image_width) & 0xFFU;
    salt[2] = (image_height >> 8) & 0xFFU;
    salt[3] = (image_height) & 0xFFU;
    salt[4] = num_components;
    salt[5] = salt[6] = salt[7] = 0;

    plen = 2*JQST_INITIAL_COEF_KDF + strlen(pass);
    pblock = malloc(plen);
    if (NULL == pblock)
        return -1;
    for (i = 0; i < JQST_INITIAL_COEF_KDF; i++)
    {
        unsigned int pi = perm[0].index[i/perm[0].pblksize]*
            perm[0].pblksize + i%perm[0].pblksize;
        JCOEF c = coef[0][pi];

        pblock[2*i] = (c >> 8) & 0xFF;
        pblock[2*i + 1] = c & 0xFF;
    }
    memcpy(pblock + 2*JQST_INITIAL_COEF_KDF, pass, plen -
            2*JQST_INITIAL_COEF_KDF);

    err1 = gcry_kdf_derive(pblock, 2*JQST_INITIAL_COEF_KDF,
            GCRY_KDF_ITERSALTED_S2K, GCRY_MD_SHA256, salt, sizeof(salt),
            65536, 16, ivec);
    err2 = gcry_kdf_derive(pblock, plen, GCRY_KDF_ITERSALTED_S2K,
            GCRY_MD_SHA256, salt, sizeof(salt), 65536, 32, key);
    memset(pblock, 0, plen);
    free(pblock);
    pblock = NULL;
    if (0 != err1 || 0 != err2) {
        if (0 != err1)
            fprintf(stderr, "deriving iv from jpeg failed: %s\n",
                    gcry_strerror(err1));
        if (0 != err2)
            fprintf(stderr, "deriving key from jpeg failed: %s\n",
                    gcry_strerror(err2));
        return -1;
    }

    return 0;
}

/* destroy and free encoded message */
static void
jqst_msg_destroy(struct jqst_message *msg)
{
    if (NULL == msg || NULL == msg->data)
        return;

    memset(msg->data, 0, msg->len);
    free(msg->data);
    msg->data = NULL;
    msg->len = msg->size = 0;
}

/* read message from file */
static int
jqst_msg_read(const char *filename, struct jqst_message *msg, size_t maxlen)
{
    FILE *file;

    memset(msg, 0, sizeof(msg[0]));
    file = fopen(filename, "rb");
    if (NULL == file)
        return -1;

    if (maxlen > 0)
        msg->size = maxlen;
    else
        msg->size = 1024;
    msg->data = malloc(msg->size*sizeof(msg->data[0]));
    if (NULL == msg->data) {
        fclose(file);
        return -1;
    }

    while (!feof(file)) {
        if (msg->len >= msg->size) {
            uint8_t *tmp;

            tmp = realloc(msg->data, msg->len + 1024);
            if (NULL == tmp) {
                jqst_msg_destroy(msg);;
                fclose(file);
                return -1;
            }
            msg->data = tmp;
            msg->size = msg->len + 1024;
        }

        msg->len += fread(msg->data + msg->len, 1, msg->size - msg->len, file);
        if (ferror(file)) {
            jqst_msg_destroy(msg);
            fclose(file);
            return -1;
        }

        if (maxlen > 0 && msg->len >= maxlen)
            break;
    }

    fclose(file);

    return 0;
}

/* encode (compress and encrypt) message from memory */
static int
jqst_msg_encode(const void *src, unsigned int srclen, const uint8_t *key,
        const uint8_t *iv, struct jqst_message *msg, unsigned int maxbytes)
{
    lzma_ret ret_xz;
    int ret = 0;
    lzma_stream strm = LZMA_STREAM_INIT;
    gcry_cipher_hd_t cipher = NULL;
    gcry_error_t err;

    msg->len = 0;
    msg->size = MAX(srclen, JQST_MIN_PAYLOAD);
    msg->data = malloc(msg->size*sizeof(msg->data[0]));
    if (NULL == msg)
        return -1;

    ret_xz = lzma_easy_encoder(&strm, LZMA_PRESET_DEFAULT, LZMA_CHECK_CRC32);
    if (LZMA_OK != ret_xz) {
        ret = -1;
        goto jqst_msg_encode_exit;
    }

    err = gcry_cipher_open(&cipher, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CFB,
            0);
    if (0 != err) {
        ret = -1;
        goto jqst_msg_encode_exit;
    }
    err = gcry_cipher_setkey(cipher, key, 32);
    if (0 != err) {
        ret = -1;
        goto jqst_msg_encode_exit;
    }
    err = gcry_cipher_setiv(cipher, iv, 16);
    if (0 != err) {
        ret = -1;
        goto jqst_msg_encode_exit;
    }

    strm.next_in = src;
    strm.avail_in = srclen;

    do {
        if (msg->len >= msg->size) {
            uint8_t *tmp;

            tmp = realloc(msg->data, (msg->len + 1024)*sizeof(msg->data[0]));
            if (NULL == tmp) {
                ret = -1;
                goto jqst_msg_encode_exit;
            }
            msg->data = tmp;
            msg->size = msg->len + 1024;
        }

        strm.next_out = msg->data + msg->len;
        strm.avail_out = msg->size - msg->len;

        ret_xz = lzma_code(&strm, LZMA_FINISH);
        if ((LZMA_OK == ret_xz) || (LZMA_STREAM_END == ret_xz)) {
            err = gcry_cipher_encrypt(cipher, msg->data + msg->len,
                    msg->size - msg->len - strm.avail_out, NULL, 0);
            if (0 != err) {
                ret = -1;
                goto jqst_msg_encode_exit;
            }
            msg->len = msg->size - strm.avail_out;
            if (msg->len > maxbytes) {
                ret = -2;
                goto jqst_msg_encode_exit;
            }
        } else {
            ret = -1;
            goto jqst_msg_encode_exit;
        }
    } while (0 == strm.avail_out);

jqst_msg_encode_exit:
    lzma_end(&strm);
    gcry_cipher_close(cipher);
    if (ret < 0) {
        jqst_msg_destroy(msg);
    }

    return ret;
}

/* Decode extracted message */
static int
jqst_msg_decode(struct jqst_message *msg, gcry_cipher_hd_t cipher)
{
    gcry_error_t err;
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret_xz;
    uint8_t *out = NULL;
    size_t out_len = 0, out_size = 0;
    int ret = 0;

    /* decrypt message */
    err = gcry_cipher_decrypt(cipher, msg->data, msg->len, NULL, 0);
    if (0 != err) {
        fprintf(stderr, "decryption of message failed\n");
        return -1;
    }

    /* decompress message */
    ret_xz = lzma_stream_decoder(&strm, JQST_MAX_DECOMP_MEM,
            LZMA_TELL_NO_CHECK | LZMA_TELL_UNSUPPORTED_CHECK);
    if (LZMA_OK != ret_xz) {
        fprintf(stderr, "initialization of decoder failed\n");
        return -1;
    }

    strm.next_in = msg->data;
    strm.avail_in = msg->len;
    out = malloc(msg->len*sizeof(msg->data[0]));
    if (NULL == out) {
        fprintf(stderr, "output buffer allocation failed\n");
        ret = -1;
        goto jqst_msg_decode_exit;
    }
    strm.next_out = out;
    strm.avail_out = out_size = msg->len;
    out_len = 0;

    do {
        if (strm.avail_out <= 0) {
            uint8_t *tmp;

            tmp = realloc(out, (out_size + 1024)*sizeof(msg->data[0]));
            if (NULL == tmp) {
                fprintf(stderr, "decoder output reallocation failed\n");
                ret = -1;
                goto jqst_msg_decode_exit;
            }
            out_size += 1024;
            out = tmp;
            strm.next_out = out + out_len;
            strm.avail_out = out_size - out_len;
        }
        ret_xz = lzma_code(&strm, LZMA_FINISH);
        if (ret_xz == LZMA_OK || ret_xz == LZMA_STREAM_END) {
            out_len = out_size - strm.avail_out;
        } else {
            ret = -1;
            goto jqst_msg_decode_exit;
        }
    } while (strm.avail_out <= 0);

jqst_msg_decode_exit:
    lzma_end(&strm);
    if (ret < 0) {
        free(out);
    } else {
        jqst_msg_destroy(msg);
        msg->data = out;
        msg->size = out_size;
        msg->len = out_len;
    }

    return ret;
}

static int
jqst_jpeg_embed(struct jqst_jpeg *jpeg, const char *pass, const void *data,
        unsigned int datalen, int verbose)
{
    int ci, ret = 0;
    unsigned int bits, i;
    uint8_t key[32], ivec[16];
    struct jqst_message msg = {0, 0, NULL};
    const jpeg_component_info *comp;
    const struct jqst_permutation *perm;

    /* stop analyzing compression pass */
    jpeg_abort_compress(&jpeg->cinfo);
    if (NULL != jpeg->fout) {
        fclose(jpeg->fout);
        jpeg->fout = NULL;
    }
    for (ci = 0; ci < jpeg->cinfo.num_components; ci++)
    {
        jpeg->qudiv_min[ci] = jpeg->qudiv[ci][0];
        for (i = 1; i < DCTSIZE2; i++)
            if (jpeg->qudiv[ci][i] < jpeg->qudiv_min[ci])
                jpeg->qudiv_min[ci] = jpeg->qudiv[ci][i];
        if (verbose)
            fprintf(stderr, "ci: %d, qudiv_min: %g\n", ci, jpeg->qudiv_min[ci]);
    }

    /* set up permutations */
    ret = jqst_jpeg_compute_permutations(jpeg->perm, jpeg->cinfo.num_components,
            jpeg->cinfo.comp_info, jpeg->cinfo.image_width,
            jpeg->cinfo.image_height, pass, JQST_DEFAULT_PBLKSIZE);
    if (ret < 0) {
        fprintf(stderr, "failed to compute permutations\n");
        return -1;
    }

    /* set up key */
    ret = jqst_jpeg_getkey(jpeg->cinfo.num_components, jpeg->cinfo.image_width,
            jpeg->cinfo.image_height, jpeg->perm, jpeg->coef, pass, key, ivec);
    if (ret < 0) {
        fprintf(stderr, "failed to derive key\n");
        return -1;
    }

    /* compress and encrypt message */
    bits = 0;
    for (ci = 0; ci < jpeg->cinfo.num_components; ci++)
    {
        const jpeg_component_info *comp = &jpeg->cinfo.comp_info[ci];

        bits += comp->width_in_blocks*comp->height_in_blocks*DCTSIZE2;
    }
    bits -= JQST_INITIAL_COEF_KDF;
    if (verbose)
        fprintf(stderr, "maximum capacity is %u bytes\n",
                bits/(JQST_MIN_BLK_SIZE*8));
    ret = jqst_msg_encode(data, datalen, key, ivec, &msg,
            bits/(JQST_MIN_BLK_SIZE*8));
    if (ret < 0) {
        fprintf(stderr, "encoding message failed\n");
        ret = -1;
        goto jqst_jpeg_embed_exit;
    }
    jpeg->blksize = bits/(msg.len*8);
    if (jpeg->blksize < JQST_MIN_BLK_SIZE) {
        fprintf(stderr, "message too large for capacity\n");
        ret = -1;
        goto jqst_jpeg_embed_exit;
    }
    if (verbose)
        fprintf(stderr, "encoded message length: %u bytes -> block size: %u coef/bit\n",
                msg.len, jpeg->blksize);

    /* embed message */
    ci = 0;
    comp = &jpeg->cinfo.comp_info[ci];
    perm = &jpeg->perm[ci];
    i = JQST_INITIAL_COEF_KDF;
    for (bits = 0; bits < 8*msg.len; bits++)
    {
        unsigned int j, pi_qemax = 0, ci_qemax = ci;
        signed char querr_max;
        uint8_t msg_bit;
        double abs_qe_flip_min;

        /* embed message bit into block */
        querr_max = 0;
        msg_bit = 0;
        abs_qe_flip_min = UINT_MAX;
        for (j = 0; j < jpeg->blksize; j++, i++)
        {
            unsigned int pi;
            /* TODO: what about used quantization divisor (jpeg->qudiv)?
             * larger divisor -> more distortion on change? */
            signed char qe;
            JCOEF coef;

            /* switch to next component when end of current one reached */
            if (i >= comp->width_in_blocks*comp->height_in_blocks*
                    DCTSIZE2) {
                if (++ci >= jpeg->cinfo.num_components) {
                    fprintf(stderr, "no capacity left at message bit %u\n",
                            bits + 1);
                    ret = -1;
                    goto jqst_jpeg_embed_exit;
                }
                comp = &jpeg->cinfo.comp_info[ci];
                perm = &jpeg->perm[ci];
                i = 0;
            }

            /* get permuted coefficient index, value and quantization
             * error */
            pi = perm->index[i/perm->pblksize]*perm->pblksize +
                i%perm->pblksize;
            qe = jpeg->querr[ci][pi];
            coef = jpeg->coef[ci][pi];

            /* update block parity and optimal absolute quantization
             * error */
            msg_bit ^= coef & 1;
            if (jpeg->optimize_absolute_querr) {
                double abs_qe_flip;

                /* minimize the absolute quantization error after a bit
                 * flip = (1.0 - |querr|)*qudiv */
                abs_qe_flip = (255U - abs(qe))*jpeg->qudiv[ci][pi%DCTSIZE2];
                if (abs_qe_flip < abs_qe_flip_min) {
                    abs_qe_flip_min = abs_qe_flip;
                    querr_max = qe;
                    pi_qemax = pi;
                    ci_qemax = ci;
                }
            } else {
                /* just consider the relative quantization error querr
                 * in [-0.5, 0.5] */
                if (abs(qe) >= abs(querr_max)) {
                    querr_max = qe;
                    pi_qemax = pi;
                    ci_qemax = ci;
                }
            }
        }
        /* block parity change needed to embed message bit? */
        if (msg_bit != ((msg.data[bits/8] >> (7 - bits%8)) & 1)) {
            JCOEF coef = jpeg->coef[ci_qemax][pi_qemax];
            unsigned int coef_type = pi_qemax%DCTSIZE2;

            if (querr_max > 0 || (0 == querr_max && rand() > RAND_MAX/2)) {
                jpeg->coef[ci_qemax][pi_qemax]++;

                if (coef + 1 <= JQST_MAX_COEF_STAT &&
                        coef >= -JQST_MAX_COEF_STAT) {
                    jpeg->coef_hist[ci_qemax][coef_type][coef +
                        JQST_MAX_COEF_STAT]--;
                    jpeg->coef_hist[ci_qemax][coef_type][coef + 1 +
                        JQST_MAX_COEF_STAT]++;
                    jpeg->coef_count[ci_qemax][coef_type]++;
                }
            } else {
                jpeg->coef[ci_qemax][pi_qemax]--;

                if (coef <= JQST_MAX_COEF_STAT &&
                        coef - 1 >= -JQST_MAX_COEF_STAT) {
                    jpeg->coef_hist[ci_qemax][coef_type][coef +
                        JQST_MAX_COEF_STAT]--;
                    jpeg->coef_hist[ci_qemax][coef_type][coef - 1 +
                        JQST_MAX_COEF_STAT]++;
                    jpeg->coef_count[ci_qemax][coef_type]++;
                }
            }
            /* change statistics */
            jpeg->querr_hist[ci_qemax][abs(querr_max)]++;
            jpeg->querr_count[ci_qemax]++;
            if (abs(querr_max) < jpeg->querr_minchg[ci_qemax])
                jpeg->querr_minchg[ci_qemax] = abs(querr_max);
        }
    }

    jpeg->msgbits = msg.len*8;

jqst_jpeg_embed_exit:
    memset(key, 0, sizeof(key));
    jqst_msg_destroy(&msg);

    return ret;
}

static void
jqst_request_virt_barray(struct jqst_jpeg *jpeg, jvirt_barray_ptr *barray)
{
    int ci;

    for (ci = 0; ci < jpeg->cinfo.num_components; ci++)
    {
        jpeg_component_info *compptr = &jpeg->cinfo.comp_info[ci];

        barray[ci] = (*jpeg->cinfo.mem->request_virt_barray)(
                (j_common_ptr)&jpeg->cinfo, JPOOL_IMAGE, FALSE,
                compptr->width_in_blocks,
                ROUND_UP(compptr->height_in_blocks, compptr->v_samp_factor),
                compptr->v_samp_factor);
    }
}

static void
jqst_fill_virt_barray(struct jqst_jpeg *jpeg, jvirt_barray_ptr *barray)
{
    int ci;

    for (ci = 0; ci < jpeg->cinfo.num_components; ci++)
    {
        jpeg_component_info *compptr = &jpeg->cinfo.comp_info[ci];
        unsigned int row;

        for (row = 0; row < compptr->height_in_blocks; row++)
        {
            JBLOCKARRAY buffer;

            buffer = (*jpeg->cinfo.mem->access_virt_barray)(
                    (j_common_ptr)&jpeg->cinfo, barray[ci], row, 1, TRUE);
            memcpy(buffer[0], &jpeg->coef[ci][row*compptr->width_in_blocks*DCTSIZE2],
                    compptr->width_in_blocks*sizeof(buffer[0][0]));
        }
        for (; row < ROUND_UP(compptr->height_in_blocks,
                    compptr->v_samp_factor); row++)
        {
            JBLOCKARRAY buffer;

            buffer = (*jpeg->cinfo.mem->access_virt_barray)(
                    (j_common_ptr)&jpeg->cinfo, barray[ci], row, 1, TRUE);
            memset(buffer[0], 0, compptr->width_in_blocks*sizeof(buffer[0][0]));
        }
    }
}

static int
jqst_read_virt_barray(struct jpeg_decompress_struct *cinfo,
        jvirt_barray_ptr *barray, JCOEF **coef)
{
    int ci;

    for (ci = 0; ci < cinfo->num_components; ci++)
    {
        const jpeg_component_info *comp = &cinfo->comp_info[ci];
        unsigned int row;

        coef[ci] = malloc(comp->width_in_blocks*comp->height_in_blocks*DCTSIZE2*
                sizeof(coef[ci][0]));
        if (NULL == coef[ci]) {
            int i;

            for (i = 0; i < ci; i++)
            {
                free(coef[i]);
                coef[i] = NULL;
            }

            return -1;
        }

        for (row = 0; row < comp->height_in_blocks; row++)
        {
            JBLOCKARRAY buffer;

            buffer = (*cinfo->mem->access_virt_barray)(
                    (j_common_ptr)cinfo, barray[ci], row, 1, FALSE);
            memcpy(&coef[ci][row*comp->width_in_blocks*DCTSIZE2], buffer[0],
                    comp->width_in_blocks*DCTSIZE2*sizeof(coef[ci][0]));
        }
    }

    return 0;
}

static int
jqst_jpeg_writefile(struct jqst_jpeg *jpeg, FILE *file)
{
    jvirt_barray_ptr barray[MAX_COMPONENTS];

    jpeg_abort_compress(&jpeg->cinfo);

    jpeg->fout = file;
    if (NULL == jpeg->fout)
        return -1;
    jpeg_stdio_dest(&jpeg->cinfo, jpeg->fout);

    jqst_request_virt_barray(jpeg, barray);
    jpeg_write_coefficients(&jpeg->cinfo, barray);
    jqst_fill_virt_barray(jpeg, barray);
    jpeg_finish_compress(&jpeg->cinfo);

    jpeg->fout = NULL;

    return 0;
}

static int
jqst_jpeg_scanline(struct jqst_jpeg *jpeg, tuple *tuplerow)
{
    unsigned int ci;
    int pi;

    for (ci = 0; ci < jpeg->cinfo.image_width; ci++)
        for (pi = 0; pi < jpeg->cinfo.input_components; pi++)
            jpeg->scanline[ci*jpeg->cinfo.input_components + pi] =
                (((unsigned long)tuplerow[ci][pi] << BITS_IN_JSAMPLE) +
                 (jpeg->maxval + 1)/2)/(jpeg->maxval + 1);

    jpeg_write_scanlines(&jpeg->cinfo, &jpeg->scanline, 1);

    return 0;
}

static int
jqst_jpeg_extract(FILE *jpeg, const char *pass, struct jqst_message *msg,
        unsigned int max_msg_len, int verbose)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    jvirt_barray_ptr *barray;
    JCOEF *coef[MAX_COMPONENTS];
    struct jqst_permutation perm[MAX_COMPONENTS];
    int ret = 0, ci;
    unsigned int i, num_coef, blksize, payload;
    uint8_t key[32], ivec[16];
    gcry_cipher_hd_t cipher = NULL;
    gcry_error_t err;

    memset(perm, 0, sizeof(perm));
    memset(coef, 0, sizeof(coef));
    memset(msg, 0, sizeof(msg[0]));

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, jpeg);
    jpeg_read_header(&cinfo, TRUE);

    ret = jqst_jpeg_compute_permutations(perm, cinfo.num_components,
        cinfo.comp_info, cinfo.image_width, cinfo.image_height, pass,
        JQST_DEFAULT_PBLKSIZE);
    if (ret < 0) {
        fprintf(stderr, "failed to generate permutation\n");
        ret = -1;
        goto jqst_jpeg_extract_exit;
    }

    barray = jpeg_read_coefficients(&cinfo);
    ret = jqst_read_virt_barray(&cinfo, barray, coef);
    if (ret < 0) {
        fprintf(stderr, "unable to read coefficients\n");
        ret = -1;
        goto jqst_jpeg_extract_exit;
    }

    ret = jqst_jpeg_getkey(cinfo.num_components, cinfo.image_width,
            cinfo.image_height, perm, coef, pass, key, ivec);
    if (ret < 0) {
        fprintf(stderr, "failed to get key\n");
        ret = -1;
        goto jqst_jpeg_extract_exit;
    }

    err = gcry_cipher_open(&cipher, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CFB,
            0);
    if (0 != err) {
        fprintf(stderr, "cipher initialization failed\n");
        ret = -1;
        goto jqst_jpeg_extract_exit;
    }

    err = gcry_cipher_setkey(cipher, key, sizeof(key));
    memset(key, 0, sizeof(key));
    if (0 != err) {
        fprintf(stderr, "setting cipher key failed\n");
        ret = -1;
        goto jqst_jpeg_extract_exit;
    }

    num_coef = 0;
    for (ci = 0; ci < cinfo.num_components; ci++)
        num_coef += cinfo.comp_info[ci].width_in_blocks*
            cinfo.comp_info[ci].height_in_blocks*DCTSIZE2;
    num_coef -= JQST_INITIAL_COEF_KDF;

    msg->size = 1024;
    msg->data = malloc(msg->size*sizeof(msg->data[0]));
    if (NULL == msg->data) {
        fprintf(stderr, "failed to allocate message memory\n");
        ret = -1;
        goto jqst_jpeg_extract_exit;
    }

    /* try possible block sizes */
    if (verbose)
        fprintf(stderr, "trying encoded message sizes in bytes:\n");
    payload = JQST_MIN_PAYLOAD;
    for (blksize = num_coef/(payload*8); blksize >= JQST_MIN_BLK_SIZE &&
            payload <= max_msg_len;
            blksize = num_coef/((payload += JQST_MSG_SIZE_DELTA)*8))
    {
        unsigned int bits;

        /* maximize payload in bytes for blocksize */
        payload = ROUND_DOWN(num_coef/(blksize*8), JQST_MSG_SIZE_DELTA);

        if (verbose) {
            fprintf(stderr, " %u", payload);
            fflush(stderr);
        }

        if (payload > msg->size) {
            uint8_t *tmp;

            tmp = realloc(msg->data, (payload + 1023)*sizeof(msg->data[0]));
            if (NULL == tmp) {
                fprintf(stderr, "\nfailed to reallocate message memory\n");
                ret = -1;
                goto jqst_jpeg_extract_exit;
            }
            msg->data = tmp;
            msg->size = payload + 1023;
        }

        msg->len = payload;
        memset(msg->data, 0, msg->len*sizeof(msg->data[0]));
        i = JQST_INITIAL_COEF_KDF;
        ci = 0;
        for (bits = 0; bits < 8*msg->len; bits++)
        {
            unsigned int j;
            int msgbit = 0;

            for (j = 0; j < blksize; j++, i++)
            {
                unsigned int pi;

                /* detect component change */
                if (i >= cinfo.comp_info[ci].width_in_blocks*
                        cinfo.comp_info[ci].height_in_blocks*DCTSIZE2) {
                    if (++ci >= cinfo.num_components) {
                        fprintf(stderr, "\ncapacity miscalculated\n");
                        ret = -1;
                        goto jqst_jpeg_extract_exit;
                    }
                    i = 0;
                }

                /* get permuted coefficient index */
                pi = perm[ci].index[i/perm[ci].pblksize]*perm[ci].pblksize +
                    i%perm[ci].pblksize;

                msgbit ^= coef[ci][pi] & 1;
            }

            msg->data[bits/8] |= (msgbit & 1) << (7 - bits%8);
        }

        /* check for valid message and decode  */
        err = gcry_cipher_reset(cipher);
        if (0 != err) {
            fprintf(stderr, "\nresetting cipher failed\n");
            ret = -1;
            goto jqst_jpeg_extract_exit;
        }
        err = gcry_cipher_setiv(cipher, ivec, sizeof(ivec));
        if (0 != err) {
            fprintf(stderr, "\nsetting cipher IV failed\n");
            ret = -1;
            goto jqst_jpeg_extract_exit;
        }

        ret = jqst_msg_decode(msg, cipher);
        if (ret >= 0) {
            if (verbose)
                fprintf(stderr, "\nmessage of size %u bytes found\n", msg->len);
            goto jqst_jpeg_extract_exit;
        }
    }
    if (verbose)
        fprintf(stderr, "\nno message found\n");

    ret = -1;
jqst_jpeg_extract_exit:
    memset(key, 0, sizeof(key));
    if (NULL != cipher)
        gcry_cipher_close(cipher);
    jpeg_destroy_decompress(&cinfo);
    for (i = 0; i < MAX_COMPONENTS; i++)
    {
        if (NULL != perm[i].index) {
            free(perm[i].index);
            perm[i].index = NULL;
        }
        if (NULL != coef[i]) {
            free(coef[i]);
            coef[i] = NULL;
        }
    }

    if (ret < 0)
        jqst_msg_destroy(msg);

    return ret;
}

static int
jqst_stat_writecoef(struct jqst_jpeg *jpeg, const char *filename)
{
    FILE *out;
    unsigned int i, j;

    out = fopen(filename, "w");
    if (NULL == out) {
        fprintf(stderr, "unable to open %s for writing\n", filename);
        return -1;
    }

    fprintf(out, "%u %u\n", jpeg->cinfo.comp_info[0].height_in_blocks,
            jpeg->cinfo.comp_info[0].width_in_blocks);

    for (i = 0; i < jpeg->cinfo.comp_info[0].height_in_blocks*
            jpeg->cinfo.comp_info[0].width_in_blocks; i++)
    {
        for (j = 0; j < DCTSIZE2; j++)
            fprintf(out, " %d", jpeg->coef[0][i*DCTSIZE2 + j]);
        fprintf(out, "\n");
    }

    fclose(out);

    return 0;
}

static int
jqst_cmd_switch(int argc, const char *argv[])
{
    int ret;

    if (argc >= 2) {
      if (0 == strcmp(argv[1], "emb") && 4 == argc) {
        struct pam inpam;
        struct jqst_jpeg jpeg;
        tuple *tuplerow;
        int row, ci;
        unsigned int i, chgbits;
        FILE *stegofile;
        struct jqst_message msg;

        /* embedding */
        pnm_readpaminit(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));

        tuplerow = pnm_allocpamrow(&inpam);

        ret = jqst_jpeg_init(&inpam, &jpeg, NULL);
        if (ret < 0) {
            fprintf(stderr, "jpeg compression object initialization failed\n");
            return -1;
        }

        for (row = 0; row < inpam.height; row++)
        {
            pnm_readpamrow(&inpam, tuplerow);
            ret = jqst_jpeg_scanline(&jpeg, tuplerow);
            if (ret < 0) {
                pnm_freepamrow(tuplerow);
                jqst_jpeg_destroy(&jpeg);
                fprintf(stderr, "processing jpeg scanline %u failed\n", row + 1);
                return -1;
            }
        }

        pnm_freepamrow(tuplerow);

        /* read message */
        ret = jqst_msg_read(argv[3], &msg, 0);
        if (ret < 0) {
            fprintf(stderr, "unable to read message from %s\n", argv[3]);
            jqst_jpeg_destroy(&jpeg);
            return -1;
        }

        /* embed data */
        ret = jqst_jpeg_embed(&jpeg, argv[2], msg.data, msg.len, 1);
        jqst_msg_destroy(&msg);
        if (ret < 0) {
            fprintf(stderr, "embedding failed\n");
            jqst_jpeg_destroy(&jpeg);
            return -1;
        }

        chgbits = 0;
        for (ci = 0; ci < jpeg.cinfo.num_components; ci++)
        {
            unsigned int ct;

            printf("\n\n#comp %d: %u changes, min changed querr: %g\n",
                    ci + 1, jpeg.querr_count[ci], jpeg.querr_minchg[ci]/255.0);
            /*for (i = 0; i < ARRAY_SIZE(jpeg.querr_hist[ci]); i++)
                printf("%g %g\n", i/255.0, (double)jpeg.querr_hist[ci][i]/
                        jpeg.querr_count[ci]);*/
            chgbits += jpeg.querr_count[ci];
            /**/printf("#");
            for (i = 0; i < ARRAY_SIZE(jpeg.coef_hist[ci][0]); i++)
                printf(" %+04d", (int)i - JQST_MAX_COEF_STAT);
            puts("");
            for (ct = 0; ct < DCTSIZE2; ct++)
            {
                printf("#DCT coef %u change count: %u\n", ct,
                        jpeg.coef_count[ci][ct]);
                for (i = 0; i < ARRAY_SIZE(jpeg.coef_hist[ci][ct]); i++)
                {
                    printf(" %+04d", jpeg.coef_hist[ci][ct][i]);
                }
                puts("");
            }/**/
        }
        fprintf(stderr, "%u coef changes; %u embedded bits\n", chgbits,
                jpeg.msgbits);

        stegofile = fopen("stego.jpg", "wb");
        if (NULL == stegofile) {
            jqst_jpeg_destroy(&jpeg);
            fprintf(stderr, "unable to write to stego.jpg\n");
            return -1;
        }
        jqst_jpeg_writefile(&jpeg, stegofile);
        fclose(stegofile);

        jqst_jpeg_destroy(&jpeg);

        return 0;
      } else if (0 == strcmp(argv[1], "ext") && 3 == argc) {
        struct jqst_message msg;

        /* extract message */
        ret = jqst_jpeg_extract(stdin, argv[2], &msg, UINT_MAX, 1);
        if (ret < 0) {
            fprintf(stderr, "failed to extract message\n");
            return -1;
        }

        printf("message:\n");
        fwrite(msg.data, msg.len, 1, stdout);
        puts("");

        jqst_msg_destroy(&msg);

        return 0;
      } else if (0 == strcmp(argv[1], "stat") && 2 == argc) {
        struct pam inpam;
        struct jqst_jpeg jpeg;
        tuple *tuplerow;
        int row, ci;
        struct jqst_message msg;
        FILE *fout;

        /* coefficient output for statistics */
        pnm_readpaminit(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));

        tuplerow = pnm_allocpamrow(&inpam);

        ret = jqst_jpeg_init(&inpam, &jpeg, NULL);
        if (ret < 0) {
            fprintf(stderr, "jpeg compression object initialization failed\n");
            return -1;
        }

        for (row = 0; row < inpam.height; row++)
        {
            pnm_readpamrow(&inpam, tuplerow);
            ret = jqst_jpeg_scanline(&jpeg, tuplerow);
            if (ret < 0) {
                pnm_freepamrow(tuplerow);
                jqst_jpeg_destroy(&jpeg);
                fprintf(stderr, "processing jpeg scanline %u failed\n", row + 1);
                return -1;
            }
        }

        pnm_freepamrow(tuplerow);

        jqst_stat_writecoef(&jpeg, "cover.stat");
        fout = fopen("cover.jpg", "wb");
        if (NULL != fout) {
            fprintf(stderr, "writing cover.jpeg\n");
            jqst_jpeg_writefile(&jpeg, fout);
            fclose(fout);
        }

        /* read message */
        msg.len = 0;
        for (ci = 0; ci < jpeg.cinfo.num_components; ci++)
        {
            jpeg_component_info *comp = &jpeg.cinfo.comp_info[ci];

            msg.len += comp->width_in_blocks*comp->height_in_blocks*DCTSIZE2;
        }
        msg.len -= JQST_INITIAL_COEF_KDF;
        msg.len /= JQST_MIN_BLK_SIZE*8;
        if (msg.len >= 60)
            msg.len -= 60;
        else
            msg.len = 0;
        ret = jqst_msg_read("/dev/urandom", &msg, msg.len);
        if (ret < 0) {
            fprintf(stderr, "unable to read random message\n");
            jqst_jpeg_destroy(&jpeg);
            return -1;
        }

        /* embed data */
        ret = jqst_jpeg_embed(&jpeg, "Test", msg.data, msg.len, 1);
        jqst_msg_destroy(&msg);
        if (ret < 0) {
            fprintf(stderr, "embedding failed\n");
            jqst_jpeg_destroy(&jpeg);
            return -1;
        }

        jqst_stat_writecoef(&jpeg, "stego.stat");
        fout = fopen("stego.jpg", "wb");
        if (NULL != fout) {
            fprintf(stderr, "writing stego.jpg\n");
            jqst_jpeg_writefile(&jpeg, fout);
            fclose(fout);
        }

        jqst_jpeg_destroy(&jpeg);

        return 0;
      }
    }

    fprintf(stderr, "embedding usage: %s emb \"<pass phrase>\" msg.txt <image.pnm\n"
            "extracting usage: %s ext \"<pass phrase>\" <image.jpg\n",
            argv[0], argv[0]);

    return -1;
}

/* CGI interface */
static void
jqst_html_out(struct jqst_html_node *tree, unsigned int indent_level)
{
    unsigned int i;
    struct jqst_html_attrib *attrib;

    for (; NULL != tree; tree = tree->next)
    {
        if (tree->tag >= ARRAY_SIZE(jqst_tag_name)) {
            fprintf(stderr, "unknown html tag type: %d\n", tree->tag);
            continue;
        }
        for (i = 0; i < indent_level; i++)
            fputs(" ", stdout);
        if (JQST_TAG_TEXT == tree->tag) {
            /* plain text output */
            if (NULL != tree->contents.text)
                fputs(tree->contents.text, stdout);
            putchar('\n');
        } else {
            /* html tag output */
            putchar('<');
            fputs(jqst_tag_name[tree->tag], stdout);
            /* tag attributes */
            for (attrib = tree->attrib; JQST_ATR_NULL != attrib->attrib; attrib++)
            {
                if (attrib->attrib >= ARRAY_SIZE(jqst_tag_attrib)) {
                    fprintf(stderr, "unknown html attribute type: %d\n", attrib->attrib);
                    continue;
                }
                putchar(' ');
                fputs(jqst_tag_attrib[attrib->attrib], stdout);
                fputs("=\"", stdout);
                switch (attrib->type) {
                    case JQST_TYPE_TEXT:
                        if (NULL != attrib->value.text)
                            fputs(attrib->value.text, stdout);
                        break;
                    case JQST_TYPE_NUMBER:
                        printf("%d", attrib->value.number);
                        break;
                    default:
                        fprintf(stderr, "unknown html attrib value type: %d\n", attrib->type);
                        break;
                }
                putchar('"');
            }
            if (NULL != tree->contents.tag) {
                /* nonempty tag */
                putchar('>');
                if (JQST_TAG_TEXT == tree->contents.tag->tag &&
                        NULL == tree->contents.tag->next) {
                    /* only text contents */
                    if (NULL != tree->contents.tag->contents.text)
                        fputs(tree->contents.tag->contents.text, stdout);
                } else {
                    /* tag contents */
                    putchar('\n');
                    jqst_html_out(tree->contents.tag, indent_level + 1);
                    for (i = 0; i < indent_level; i++)
                        fputs(" ", stdout);
                }
                fputs("</", stdout);
                fputs(jqst_tag_name[tree->tag], stdout);
                fputs(">\n", stdout);
            } else {
                /* empty tag */
                fputs(" />\n", stdout);
            }
        }
    }
}

static struct jqst_html_node *
jqst_html_new(enum jqst_html_tag tag, const char *atypes, ...)
{
    va_list ap;
    unsigned int num, i;
    struct jqst_html_node *node;

    if (NULL == atypes)
        num = 0;
    else
        num = strlen(atypes);
    node = malloc(sizeof(node[0]) + num*sizeof(node->attrib[0]));
    if (NULL == node)
        return NULL;

    node->tag = tag;
    node->contents.tag = NULL;
    node->next = NULL;

    va_start(ap, atypes);
    for(i = 0; i < num; i++)
    {
        node->attrib[i].attrib = (enum jqst_tag_attrib) va_arg(ap, int);
        switch (atypes[i]) {
            case 't':
                node->attrib[i].type = JQST_TYPE_TEXT;
                node->attrib[i].value.text = va_arg(ap, const char *);
                break;
            case 'n':
                node->attrib[i].type = JQST_TYPE_NUMBER;
                node->attrib[i].value.number = va_arg(ap, int);
                break;
            default:
                fprintf(stderr, "jqst_html_new: unknown type: %c\n", atypes[i]);
                node->attrib[i].type = JQST_TYPE_TEXT;
                node->attrib[i].value.text = NULL;
                (void)va_arg(ap, const char *);
                break;
        }
    }
    va_end(ap);

    node->attrib[i].attrib = JQST_ATR_NULL;
    node->attrib[i].type = JQST_TYPE_TEXT;
    node->attrib[i].value.text = NULL;

    return node;
}

static void
jqst_html_free(struct jqst_html_node *tree)
{
    struct jqst_html_node *next = NULL;

    for (; NULL != tree; tree = next)
    {
        if (JQST_TAG_TEXT != tree->tag &&
                NULL != tree->contents.tag)
            jqst_html_free(tree->contents.tag);
        next = tree->next;
        free(tree);
    }
}

static struct jqst_html_node *
jqst_html_newframe(struct jqst_html_node ***head, struct jqst_html_node ***body)
{
    struct jqst_html_node *html;

    html = jqst_html_new(JQST_TAG_HTML, NULL);
    if (NULL == html)
        return NULL;
    html->contents.tag = jqst_html_new(JQST_TAG_HEAD, NULL);
    if (NULL == html->contents.tag) {
        jqst_html_free(html);
        return NULL;
    }
    html->contents.tag->next = jqst_html_new(JQST_TAG_BODY, NULL);
    if (NULL == html->contents.tag->next) {
        jqst_html_free(html);
        return NULL;
    }

    if (head != NULL)
        head[0] = &html->contents.tag->contents.tag;
    if (body != NULL)
        body[0] = &html->contents.tag->next->contents.tag;

    return html;
}

static struct jqst_html_node *
jqst_html_newhead(const char *title, const char *csslink)
{
    struct jqst_html_node *head;

    head = jqst_html_new(JQST_TAG_META, "tt", JQST_ATR_HTTP_EQUIV,
            "Content-Style-Type", JQST_ATR_CONTENT, "text/css");
    if (NULL == head) {
        return NULL;
    }
    head->next = jqst_html_new(JQST_TAG_LINK, "ttt", JQST_ATR_REL,
            "stylesheet", JQST_ATR_TYPE, "text/css", JQST_ATR_HREF,
            csslink);
    if (NULL == head->next) {
        jqst_html_free(head);
        return NULL;
    }
    head->next->next = jqst_html_new(JQST_TAG_TITLE, NULL);
    if (NULL == head->next->next) {
        jqst_html_free(head);
        return NULL;
    }
    head->next->next->contents.tag = jqst_html_new(JQST_TAG_TEXT, NULL);
    if (NULL == head->next->next->contents.tag) {
        jqst_html_free(head);
        return NULL;
    }
    head->next->next->contents.tag->contents.text = title;

    return head;
}

static struct jqst_html_node *
jqst_html_newformframe(const char *action, const char *legend,
        struct jqst_html_node ***contents)
{
    struct jqst_html_node *form;

    form = jqst_html_new(JQST_TAG_FORM, "ttt", JQST_ATR_METHOD, "post",
            JQST_ATR_ENCTYPE, "multipart/form-data", JQST_ATR_ACTION, action);
    if (NULL == form) {
        return NULL;
    }
    form->contents.tag = jqst_html_new(JQST_TAG_FIELDSET, NULL);
    if (NULL == form->contents.tag) {
        jqst_html_free(form);
        return NULL;
    }
    form->contents.tag->contents.tag = jqst_html_new(JQST_TAG_LEGEND, NULL);
    if (NULL == form->contents.tag->contents.tag) {
        jqst_html_free(form);
        return NULL;
    }
    form->contents.tag->contents.tag->contents.tag = jqst_html_new(JQST_TAG_TEXT, NULL);
    if (NULL == form->contents.tag->contents.tag->contents.tag) {
        jqst_html_free(form);
        return NULL;
    }
    form->contents.tag->contents.tag->contents.tag->contents.text = legend;
    if (NULL != contents)
        contents[0] = &form->contents.tag->contents.tag->next;

    return form;
}

static struct jqst_html_node *
jqst_html_buildtdnode(unsigned int span, enum jqst_align align)
{
    struct jqst_html_node *td;
    unsigned int atr = 0;

    td = jqst_html_new(JQST_TAG_TD, "tt", JQST_ATR_NULL, NULL,
            JQST_ATR_NULL, NULL);
    if (NULL == td)
        return NULL;
    if (span > 1) {
        td->attrib[atr].attrib = JQST_ATR_COLSPAN;
        td->attrib[atr].type = JQST_TYPE_NUMBER;
        td->attrib[atr++].value.number = span;
    }
    if (JQST_ALIGN_NONE != align) {
        const char *talign;

        td->attrib[atr].attrib = JQST_ATR_ALIGN;
        td->attrib[atr].type = JQST_TYPE_TEXT;
        switch (align) {
            case JQST_ALIGN_LEFT:
                talign = "left";
                break;
            case JQST_ALIGN_RIGHT:
                talign = "right";
                break;
            case JQST_ALIGN_CENTER:
                talign = "center";
                break;
            case JQST_ALIGN_BOTTOM:
                talign = "bottom";
                break;
            default:
                fprintf(stderr, "jqst_html_buildformtable: unknown alignment: %d\n",
                        align);
                talign = "";
                break;
        }
        td->attrib[atr++].value.text = talign;
    }

    return td;
}

static struct jqst_html_node *
jqst_html_buildformtable(const struct jqst_html_formtable *table)
{
    struct jqst_html_node *result, *curtr, **nexttd;
    const struct jqst_html_formtable_cell *cell;

    if (NULL == table)
        return NULL;

    result = jqst_html_new(JQST_TAG_TABLE, "n", JQST_ATR_CELLPADDING,
            table->pad);
    if (NULL == result)
        return NULL;

    result->contents.tag = curtr = jqst_html_new(JQST_TAG_TR, NULL);
    if (NULL == curtr) {
        jqst_html_free(result);
        return NULL;
    }
    nexttd = &curtr->contents.tag;
    for (cell = table->cell; JQST_TBL_END != cell->type; cell++)
    {
        if (JQST_TBL_NEWROW == cell->type) {
            curtr->next = jqst_html_new(JQST_TAG_TR, NULL);
            if (NULL == curtr->next) {
                jqst_html_free(result);
                return NULL;
            }
            curtr = curtr->next;
            nexttd = &curtr->contents.tag;
            continue;
        }

        nexttd[0] = jqst_html_buildtdnode(cell->span, cell->align);
        if (NULL == nexttd[0]) {
            jqst_html_free(result);
            return NULL;
        }

        switch (cell->type) {
            struct jqst_html_node *input, **nextopt;
            const char *type, **opt;
            unsigned int atr;

            case JQST_TBL_INPUT:
                switch (cell->tspec.input.type) {
                    case JQST_ITYPE_FILE:
                        type = "file";
                        break;
                    case JQST_ITYPE_PASSWORD:
                        type = "password";
                        break;
                    case JQST_ITYPE_TEXT:
                        type = "text";
                        break;
                    case JQST_ITYPE_CHECKBOX:
                        type = "checkbox";
                        break;
                    case JQST_ITYPE_RADIO:
                        type = "radio";
                        break;
                    case JQST_ITYPE_SUBMIT:
                        type = "submit";
                        break;
                    case JQST_ITYPE_RESET:
                        type = "reset";
                        break;
                    default:
                        fprintf(stderr, "jqst_html_buildformtable: unkown input type: %d\n",
                                cell->tspec.input.type);
                        type = "";
                        break;
                }
                input = jqst_html_new(JQST_TAG_INPUT, "tttttt",
                        JQST_ATR_TYPE, type, JQST_ATR_NAME, cell->name,
                        JQST_ATR_ID, cell->id, JQST_ATR_NULL, NULL,
                        JQST_ATR_NULL, NULL, JQST_ATR_NULL, NULL);
                nexttd[0]->contents.tag = input;
                if (NULL == input) {
                    jqst_html_free(result);
                    return NULL;
                }
                atr = 3;
                if (NULL != cell->value) {
                    input->attrib[atr].attrib = JQST_ATR_VALUE;
                    input->attrib[atr].type = JQST_TYPE_TEXT;
                    input->attrib[atr++].value.text = cell->value;
                }
                if (cell->size > 0) {
                    input->attrib[atr].attrib = JQST_ATR_SIZE;
                    input->attrib[atr].type = JQST_TYPE_NUMBER;
                    input->attrib[atr++].value.number = cell->size;
                }
                if ((JQST_ITYPE_CHECKBOX == cell->tspec.input.type ||
                            JQST_ITYPE_RADIO == cell->tspec.input.type) &&
                        cell->tspec.input.checked)
                {
                    input->attrib[atr].attrib = JQST_ATR_CHECKED;
                    input->attrib[atr].type = JQST_TYPE_TEXT;
                    input->attrib[atr++].value.text = "checked";
                }
                break;
            case JQST_TBL_TEXTAREA:
                input = jqst_html_new(JQST_TAG_TEXTAREA, "ttnn", JQST_ATR_NAME,
                        cell->name, JQST_ATR_ID, cell->id, JQST_ATR_ROWS,
                        cell->tspec.textarea.rows, JQST_ATR_COLS, cell->tspec.textarea.cols);
                if (NULL == input) {
                    jqst_html_free(result);
                    return NULL;
                }
                nexttd[0]->contents.tag = input;
                if (NULL != cell->value) {
                    input->contents.tag = jqst_html_new(JQST_TAG_TEXT, NULL);
                    if (NULL == input->contents.tag) {
                        jqst_html_free(result);
                        return NULL;
                    }
                    input->contents.tag->contents.text = cell->value;
                }
                break;
            case JQST_TBL_SELECT:
                input = jqst_html_new(JQST_TAG_SELECT, "ttn", JQST_ATR_NAME,
                        cell->name, JQST_ATR_ID, cell->id, JQST_ATR_SIZE, cell->size);
                if (NULL == input) {
                    jqst_html_free(result);
                    return NULL;
                }
                nexttd[0]->contents.tag = input;
                nextopt = &input->contents.tag;
                for (opt = cell->tspec.select.opt; NULL != opt[0]; opt++)
                {
                    struct jqst_html_node *option;

                    option = jqst_html_new(JQST_TAG_OPTION, NULL);
                    if (NULL == option) {
                        jqst_html_free(result);
                        return NULL;
                    }
                    nextopt[0] = option;
                    option->contents.tag = jqst_html_new(JQST_TAG_TEXT, NULL);
                    if (NULL == option->contents.tag) {
                        jqst_html_free(result);
                        return NULL;
                    }
                    option->contents.tag->contents.text = opt[0];
                    nextopt = &option->next;
                }
                break;
            default:
                fprintf(stderr, "jqst_html_buildformtable: unknown table element type: %d\n",
                        cell->type);
                break;
        }

        if (NULL != cell->label) {
            /* build label */
            struct jqst_html_node **plabel, *label;

            if (cell->label_span > 0) {
                struct jqst_html_node *labeltd;

                labeltd = jqst_html_buildtdnode(cell->label_span,
                        cell->label_align);
                if (NULL == labeltd) {
                    jqst_html_free(result);
                    return NULL;
                }
                plabel = &labeltd->contents.tag;
                if (cell->label_behind && NULL != nexttd[0]) {
                    nexttd[0]->next = labeltd;
                } else {
                    labeltd->next = nexttd[0];
                    nexttd[0] = labeltd;
                }
                nexttd = &nexttd[0]->next;
            } else {
                plabel = &nexttd[0]->contents.tag;
                if (cell->label_behind)
                    while (plabel[0] != NULL)
                        plabel = &plabel[0]->next;
            }

            label = jqst_html_new(JQST_TAG_LABEL, "t",
                    JQST_ATR_FOR, cell->id);
            if (NULL == label) {
                jqst_html_free(result);
                return NULL;
            }

            label->next = plabel[0];
            plabel[0] = label;

            label->contents.tag = jqst_html_new(JQST_TAG_TEXT, NULL);
            if (NULL == label->contents.tag) {
                jqst_html_free(result);
                return NULL;
            }
            label->contents.tag->contents.text = cell->label;
        }

        /* advance to next table data element */
        nexttd = &nexttd[0]->next;
    }

    return result;
}

static void
jqst_cgi_header(const char *contenttype, const char *filename, ...)
{
    printf("Content-Type: %s\r\n", contenttype);
    if (NULL != filename) {
        va_list ap;

        printf("Content-Disposition: attachment; filename=\"");
        va_start(ap, filename);
        vprintf(filename, ap);
        va_end(ap);
        printf("\"\r\n");
    }
    printf("\r\n");
}

static void
jqst_cgi_print_form(const char *title, const char *text, const char *action,
        const char *path_info, const char *legend,
        const struct jqst_html_formtable *formdef)
{
    struct jqst_html_node *html, **head, **body, **form;
    char *action_str;
    unsigned int action_len = 0, path_info_len = 0;

    html = jqst_html_newframe(&head, &body);
    if (NULL == html)
        return;
    head[0] = jqst_html_newhead(title, "../style.css");
    body[0] = jqst_html_new(JQST_TAG_TEXT, NULL);
    if (NULL != body[0]) {
        body[0]->contents.text = text;
        body = &body[0]->next;
    }

    if (NULL != action)
        action_len = strlen(action);
    if (NULL != path_info)
        path_info_len = strlen(path_info);
    action_str = malloc((action_len + path_info_len + 1)*sizeof(action_str[0]));
    if (NULL == action_str) {
        jqst_html_free(html);
        return;
    }
    memcpy(action_str, action, action_len*sizeof(action[0]));
    memcpy(action_str + action_len, path_info, path_info_len*sizeof(path_info[0]));
    action_str[action_len + path_info_len] = 0;
    body[0] = jqst_html_newformframe(action_str, legend, &form);
    if (NULL == body[0]) {
        free(action_str);
        jqst_html_free(html);
        return;
    }

    form[0] = jqst_html_buildformtable(formdef);

    jqst_html_out(html, 0);

    free(action_str);
    jqst_html_free(html);
}

static void
jqst_cgi_print_error(const char *msg, ...)
{
    va_list ap;

    printf("<html><head><title>Error</title></head>\n"
            "<body><h1>An error occurred</h1><p>");
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    printf("</p></body></html>\n");
}

static int
jqst_cgi_embed(FILE *image, const char *pass, const char *msg,
        struct jqst_options *opt)
{
    struct pam inpam;
    struct jqst_jpeg jpeg;
    tuple *tuplerow;
    int row, ret, ci;
    unsigned int msglen, i;
    uint32_t qe_avg, qe_count;
    unsigned char qe_min;

    pnm_readpaminit(image, &inpam, PAM_STRUCT_SIZE(tuple_type));

    tuplerow = pnm_allocpamrow(&inpam);

    ret = jqst_jpeg_init(&inpam, &jpeg, opt);
    if (ret < 0) {
        pnm_freepamrow(tuplerow);
        jqst_cgi_header("text/html", NULL);
        jqst_cgi_print_error("Could not initialize JPEG compression");
        return -1;
    }

    for (row = 0; row < inpam.height; row++)
    {
        pnm_readpamrow(&inpam, tuplerow);
        ret = jqst_jpeg_scanline(&jpeg, tuplerow);
        if (ret < 0) {
            pnm_freepamrow(tuplerow);
            jqst_jpeg_destroy(&jpeg);
            jqst_cgi_header("text/html", NULL);
            jqst_cgi_print_error("Could not process a line of image data");
            return -1;
        }
    }

    pnm_freepamrow(tuplerow);

    /* embed data */
    if (NULL == msg)
        msg = "";
    msglen = strlen(msg);
    if (msglen > JQST_MAX_CGI_MSG_LEN)
        msglen = JQST_MAX_CGI_MSG_LEN;

    if (NULL == pass)
        pass = "";

    ret = jqst_jpeg_embed(&jpeg, pass, msg, msglen, 0);
    if (ret < 0) {
        jqst_jpeg_destroy(&jpeg);
        jqst_cgi_header("text/html", NULL);
        jqst_cgi_print_error("Embedding operation failed");
        return -1;
    }

    /* statistics for filename */
    qe_avg = qe_count = 0;
    qe_min = jpeg.querr_minchg[0];
    for (ci = 0; ci < jpeg.cinfo.num_components; ci++)
    {
        if (jpeg.querr_minchg[ci] < qe_min)
            qe_min = jpeg.querr_minchg[ci];
        for (i = 0; i < ARRAY_SIZE(jpeg.querr_hist[ci]); i++)
        {
            qe_count += jpeg.querr_hist[ci][i];
            qe_avg += i*jpeg.querr_hist[ci][i];
        }
    }
    if (qe_count <= 0)
        qe_count = 1;

    jqst_cgi_header("application/octet-stream",
            "stego-blk%u-qemin%.2f-qeavg%.2f.jpg", jpeg.blksize, qe_min/255.0,
            qe_avg/(255.0*qe_count));
    jqst_jpeg_writefile(&jpeg, stdout);

    jqst_jpeg_destroy(&jpeg);

    return 0;
}

static int
jqst_cgi_extract(FILE *image, const char *pass)
{
    int ret;
    struct jqst_message msg;

    if (NULL == pass)
        pass = "";
    ret = jqst_jpeg_extract(image, pass, &msg, JQST_MAX_CGI_MSG_LEN, 0);
    if (ret < 0) {
        jqst_cgi_header("text/html", NULL);
        jqst_cgi_print_error("No embedded message for given pass phrase found");
        return -1;
    }

    jqst_cgi_header("text/plain", NULL);
    fwrite(msg.data, msg.len, 1, stdout);

    jqst_msg_destroy(&msg);

    return 0;
}

static int
jqst_cgi_parseopt(CGI_varlist *cgi, struct jqst_options *opt)
{
    CGI_value *options;
    const char *optstr;

    /* defaults */
    opt->arith = FALSE;
    opt->optimize = FALSE;
    opt->smooth = 0;
    opt->dct = JDCT_ISLOW;
    opt->quality = 85;

    options = CGI_lookup_all(cgi, "options");
    if (NULL != options) {
        unsigned int i;

        for (i = 0; NULL != options[i]; i++)
        {
            if (0 == strcmp(options[i], "arith")) {
                opt->arith = TRUE;
                /*fprintf(stderr, "option: arithmetic coding\n");*/
            } else if (0 == strcmp(options[i], "optimize")) {
                opt->optimize = TRUE;
                /*fprintf(stderr, "option: optimized coding\n");*/
            } else {
                jqst_cgi_header("text/html", NULL);
                jqst_cgi_print_error("Invalid option selected");
                return -1;
            }
        }
    }
    optstr = CGI_lookup(cgi, "smooth");
    if (NULL != optstr) {
        int ret;

        ret = jqst_parse_number(optstr, &opt->smooth);
        if (ret < 0 || opt->smooth < 0 || opt->smooth > 100) {
            jqst_cgi_header("text/html", NULL);
            jqst_cgi_print_error("Smoothing must be in the range from 0 to 100");
            return -1;
        }
        /*fprintf(stderr, "option: smoothing = %d\n", opt->smooth);*/
    }
    optstr = CGI_lookup(cgi, "quality");
    if (NULL != optstr) {
        int ret;

        ret = jqst_parse_number(optstr, &opt->quality);
        if (ret < 0 || opt->quality < 0 || opt->quality > 100) {
            jqst_cgi_header("text/html", NULL);
            jqst_cgi_print_error("Quality must be in the range from 0 to 100");
            return -1;
        }
        /*fprintf(stderr, "option: quality = %d\n", opt->quality);*/
    }
    optstr = CGI_lookup(cgi, "dct_method");
    if (NULL != optstr) {
        if (0 == strcmp(optstr, JQST_DCTSTR_ISLOW)) {
            opt->dct = JDCT_ISLOW;
            /*fprintf(stderr, "option: integer slow dct\n");*/
        } else if (0 == strcmp(optstr, JQST_DCTSTR_IFAST)) {
            opt->dct = JDCT_IFAST;
            /*fprintf(stderr, "option: integer fast dct\n");*/
        } else if (0 == strcmp(optstr, JQST_DCTSTR_FLOAT)) {
            opt->dct = JDCT_FLOAT;
            /*fprintf(stderr, "option: floating point dct\n");*/
        } else {
            jqst_cgi_header("text/html", NULL);
            jqst_cgi_print_error("Invalid DCT method selected");
            return -1;
        }
    }

    return 0;
}

static int
jqst_cgi_switch(CGI_varlist *cgi, FILE *image)
{
    const char *method, *script, *passphr, *action;
    enum {
        JQST_METHOD_GET,
        JQST_METHOD_POST
    } request;
    struct jqst_html_formtable ftable = {.pad = 5, .cell = NULL};
    int ret = 0;

    script = getenv("SCRIPT_NAME");
    if (NULL == script) {
        jqst_cgi_header("text/html", NULL);
        jqst_cgi_print_error("No script name given");
        return -1;
    }

    method = getenv("REQUEST_METHOD");
    if (NULL == method || 0 == strcmp(method, "GET"))
        request = JQST_METHOD_GET;
    else if (0 == strcmp(method, "POST"))
        request = JQST_METHOD_POST;
    else {
        jqst_cgi_header("text/html", NULL);
        jqst_cgi_print_error("Invalid request type");
        return -1;
    }

    action = CGI_lookup(cgi, "action");
    if (NULL == action) {
        jqst_cgi_header("text/html", NULL);
        jqst_cgi_print_error("No action type specified");
        return -1;
    } else if (0 == strcmp(action, "embed")) {
        if (JQST_METHOD_GET == request) {
            /* deliver html form for embedding */
            jqst_cgi_header("text/html", NULL);
            ftable.cell = jqst_formcells_embed;
            jqst_cgi_print_form("JQStego - Embed a secret message",
                    "<h1>Message hiding form</h1>", script, "?action=embed",
                    "JQStego", &ftable);
        } else {
            const char *message;
            struct jqst_options opt;

            /* process embedding request */
            if (NULL == image) {
                jqst_cgi_header("text/html", NULL);
                jqst_cgi_print_error("There was no image file uploaded");
                return -1;
            }
            message = CGI_lookup(cgi, "message");
            passphr = CGI_lookup(cgi, "passphr");
            ret = jqst_cgi_parseopt(cgi, &opt);
            if (ret < 0)
                return -1;

            ret = jqst_cgi_embed(image, passphr, message, &opt);
        }
    } else if (0 == strcmp(action, "extract")) {
        if (JQST_METHOD_GET == request) {
            /* deliver html form for extraction */
            jqst_cgi_header("text/html", NULL);
            ftable.cell = jqst_formcells_extract;
            jqst_cgi_print_form("JQStego - Extract a secret message",
                    "<h1>Message extraction form</h1>", script, "?action=extract",
                    "JQStego", &ftable);
        } else {
            /* process extraction request */
            if (NULL == image) {
                jqst_cgi_header("text/html", NULL);
                jqst_cgi_print_error("There was no image file uploaded");
                return -1;
            }
            passphr = CGI_lookup(cgi, "passphr");

            ret = jqst_cgi_extract(image, passphr);
        }
    } else {
        jqst_cgi_header("text/html", NULL);
        jqst_cgi_print_error("Invalid action type");
        return -1;
    }

    return ret;
}

static FILE *
jqst_cgi_getfile(CGI_varlist *cgi, const char *name, const char *prefix)
{
    CGI_value *value;
    unsigned int i, plen;
    FILE *file = NULL;

    value = CGI_lookup_all(cgi, name);
    if (NULL == value)
        return NULL;

    plen = strlen(prefix);
    for (i = 0; NULL != value[i]; i++)
    {
        if (strlen(value[i]) == plen + 6 && 0 == strncmp(prefix, value[i],
                    plen)) {
            /*fprintf(stderr, "found file for name %s at %s\n", name, value[i]);*/
            file = fopen(value[i], "rb");
            if (NULL != file)
                break;
        }
    }

    return file;
}

static void
jqst_cgi_unlinkall(CGI_varlist *cgi, const char *prefix)
{
    const char *name;
    unsigned int plen;

    plen = strlen(prefix);
    for (name = CGI_first_name(cgi); NULL != name; name = CGI_next_name(cgi))
    {
        CGI_value *value;
        unsigned int i;

        value = CGI_lookup_all(cgi, NULL);
        for (i = 0; NULL != value[i]; i++)
        {
            if (strlen(value[i]) == plen + 6 && 0 == strncmp(prefix, value[i],
                        plen)) {
                /*fprintf(stderr, "removing file %s for %s\n", value[i], name);*/
                remove(value[i]);
            }
        }
    }
}

static int
jqst_cgi_setrlimit(int type, rlim_t value)
{
    struct rlimit limit;

    limit.rlim_cur = value;
    limit.rlim_max = value;
    if (0 != setrlimit(type, &limit)) {
        fprintf(stderr, "setrlimit for type %d failed\n", type);
        return -1;
    }

    return 0;
}

/*** Main program ***/
int
main(int argc, const char *argv[])
{
    int ret;

    /* initialization */
    if (NULL == gcry_check_version(GCRYPT_VERSION)) {
        fprintf(stderr, "failed to initialize libgcrypt\n");
        return EXIT_FAILURE;
    }
    gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
    pm_init(argv[0], 0);

    /* check for CGI */
    if (NULL != getenv("GATEWAY_INTERFACE")) {
        CGI_varlist *cgi;
        FILE *imgfile;

        /* set resource limits for cgi */
        /*jqst_cgi_setrlimit(RLIMIT_CPU, JQST_CGI_LIMIT_CPU);*/
        jqst_cgi_setrlimit(RLIMIT_FSIZE, JQST_CGI_LIMIT_FSIZE);

        /* parse all CGI input */
        cgi = CGI_get_all(JQST_TMP_PREFIX "XXXXXX");
        /* open image file in temporary directory, if available */
        imgfile = jqst_cgi_getfile(cgi, "imgfile", JQST_TMP_PREFIX);
        /* unlink all transmitted files in temporary directory */
        jqst_cgi_unlinkall(cgi, JQST_TMP_PREFIX);

        /* process CGI request */
        ret = jqst_cgi_switch(cgi, imgfile);
        if (NULL != imgfile)
            fclose(imgfile);
        CGI_free_varlist(cgi);
        if (ret < 0)
            return EXIT_FAILURE;
    } else {
        /* normal command line mode */
        ret = jqst_cmd_switch(argc, argv);
        if (ret < 0)
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
