#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/kprobes.h>

#include <drm/drm_edid.h>

MODULE_AUTHOR("jnspr");
MODULE_LICENSE("GPL");

struct replacement {
    const char *display_name;
    size_t replacement_size;
    const uint8_t *replacement_edid;
};

// Declare a struct for each replacement in fixes.h
#define DECLARE_REPLACEMENT(name, _display_name, ...)                  \
    static const uint8_t name##_replacement_edid[] = { __VA_ARGS__ };  \
    static struct replacement name = {                                 \
        .display_name = _display_name,                                 \
        .replacement_size = sizeof(name##_replacement_edid),           \
        .replacement_edid = name##_replacement_edid,                   \
    };
# include "fixes.h"
#undef DECLARE_REPLACEMENT

// Use the same header to add them to a NULL-terminated list of replacements
const struct replacement *replacements[] = {
#define DECLARE_REPLACEMENT(name, _display_name, ...) &name,
# include "fixes.h"
#undef DECLARE_REPLACEMENT
    NULL
};

// Source: https://en.wikipedia.org/wiki/Extended_Display_Identification_Data#Display_Descriptors
#define EDID_STR_MAX 13

static void sanitize_edid_string(char result[EDID_STR_MAX + 1], u8 input[EDID_STR_MAX]) {
    char *result_pos = &result[0];
    size_t index;

    for (index = 0; index < EDID_STR_MAX; index++) {
        if (input[index] == 0x0a) {
            // Stop on LF
            break;
        }
        if (isprint(input[index])) {
            // Only include printable characters
            *(result_pos++) = input[index];
        }
    }
    *result_pos = '\0';
}

static int match_display_name(struct edid *edid, const char *name) {
    struct detailed_timing *detailed;
    char string[EDID_STR_MAX + 1];
    size_t index;

    for (index = 0; index < ARRAY_SIZE(edid->detailed_timings); index++) {
        detailed = &edid->detailed_timings[index];

        // Only work with Display name (0xFC) descriptors
        if (detailed->pixel_clock != 0 || detailed->data.other_data.type != 0xfc) {
            continue;
        }

        sanitize_edid_string(string, detailed->data.other_data.data.str.str);
        if (strcmp(string, name) == 0) {
            return 0;
        }
    }
    return -1;
}

static int on_get_edid_return(struct kretprobe_instance *instance, struct pt_regs *regs) {
    struct edid *edid, *orig_edid = (void *) regs_return_value(regs);
    const struct replacement *replacement;

    if (orig_edid == NULL) {
        return 0;
    }

    for (size_t index = 0; replacements[index] != NULL; index++) {
        replacement = replacements[index];

        if (match_display_name(orig_edid, replacement->display_name) == 0) {
            edid = kmemdup(replacement->replacement_edid, replacement->replacement_size, GFP_KERNEL);
            if (edid == NULL) {
                // Better to use the original EDID instead of just returning NULL here
                return 0;
            }

            // Free the original EDID and replace it
            kfree(orig_edid);
            regs_set_return_value(regs, (uintptr_t) edid);
            return 0;
        }
    }

    return 0;
}

static struct kretprobe get_edid_probe = {
    .kp = {
        .symbol_name = "_drm_do_get_edid",
    },
    .handler = on_get_edid_return,
    .maxactive = 16,
};

static int on_init(void) {
    return register_kretprobe(&get_edid_probe);
}

static void on_exit(void) {
    unregister_kretprobe(&get_edid_probe);
}

module_init(on_init);
module_exit(on_exit);
