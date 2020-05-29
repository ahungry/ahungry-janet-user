#include <janet.h>
static const unsigned char bytes[] = {215, 205, 0, 153, 0, 0, 6, 0, 0, 205, 127, 255, 255, 255, 6, 12, 206, 4, 109, 97, 105, 110, 206, 10, 117, 115, 101, 114, 46, 106, 97, 110, 101, 116, 215, 205, 0, 152, 0, 0, 4, 0, 0, 0, 3, 8, 206, 4, 105, 110, 105, 116, 206, 46, 47, 117, 115, 114, 47, 108, 111, 99, 97, 108, 47, 108, 105, 98, 47, 106, 97, 110, 101, 116, 47, 99, 111, 109, 46, 97, 104, 117, 110, 103, 114, 121, 46, 103, 117, 105, 46, 105, 117, 112, 46, 106, 97, 110, 101, 116, 216, 14, 95, 48, 48, 48, 48, 49, 98, 105, 110, 116, 45, 112, 116, 114, 216, 15, 95, 48, 48, 48, 48, 49, 98, 99, 104, 97, 114, 45, 112, 116, 114, 216, 14, 95, 48, 48, 48, 48, 49, 98, 73, 117, 112, 79, 112, 101, 110, 44, 0, 0, 0, 42, 2, 0, 0, 51, 1, 2, 0, 42, 3, 1, 0, 51, 2, 3, 0, 48, 1, 2, 0, 42, 3, 2, 0, 52, 3, 0, 0, 5, 1, 1, 12, 0, 12, 0, 22, 0, 22, 0, 3, 0, 3, 0, 3, 206, 12, 77, 121, 32, 102, 105, 114, 115, 116, 32, 71, 85, 73, 206, 22, 71, 114, 101, 101, 116, 105, 110, 103, 115, 32, 102, 114, 111, 109, 32, 116, 104, 101, 32, 71, 85, 73, 216, 17, 95, 48, 48, 48, 48, 49, 98, 73, 117, 112, 77, 101, 115, 115, 97, 103, 101, 216, 18, 95, 48, 48, 48, 48, 49, 98, 73, 117, 112, 77, 97, 105, 110, 76, 111, 111, 112, 216, 15, 95, 48, 48, 48, 48, 49, 98, 73, 117, 112, 67, 108, 111, 115, 101, 44, 1, 0, 0, 42, 3, 0, 0, 51, 2, 3, 0, 42, 3, 1, 0, 42, 4, 2, 0, 48, 3, 4, 0, 42, 4, 3, 0, 51, 3, 4, 0, 42, 5, 4, 0, 51, 4, 5, 0, 42, 5, 5, 0, 52, 5, 0, 0, 3, 1, 1, 3, 0, 3, 1, 3, 0, 3, 0, 3, 0, 3, 0, 3, 1, 3, 0, 3, 1, 3, 0, 3};

const unsigned char *janet_payload_image_embed = bytes;
size_t janet_payload_image_embed_size = sizeof(bytes);
extern void janet_module_entry_com_ahungry_iup(JanetTable *);

int main(int argc, const char **argv) {
    janet_init();

    /* Get core env */
    JanetTable *env = janet_core_env(NULL);
    JanetTable *lookup = janet_env_lookup(env);
    JanetTable *temptab;
    int handle = janet_gclock();

    /* Load natives into unmarshalling dictionary */

        temptab = janet_table(0);
    temptab->proto = env;
    janet_module_entry_com_ahungry_iup(temptab);
    janet_env_lookup_into(lookup, temptab, "_00001b", 0);

    /* Unmarshal bytecode */
    Janet marsh_out = janet_unmarshal(
      janet_payload_image_embed,
      janet_payload_image_embed_size,
      0,
      lookup,
      NULL);

    /* Verify the marshalled object is a function */
    if (!janet_checktype(marsh_out, JANET_FUNCTION)) {
        fprintf(stderr, "invalid bytecode image - expected function.");
        return 1;
    }
    JanetFunction *jfunc = janet_unwrap_function(marsh_out);

    /* Check arity */
    janet_arity(argc, jfunc->def->min_arity, jfunc->def->max_arity);

    /* Collect command line arguments */
    JanetArray *args = janet_array(argc);
    for (int i = 0; i < argc; i++) {
        janet_array_push(args, janet_cstringv(argv[i]));
    }

    /* Create enviornment */
    temptab = janet_table(0);
    temptab = env;
    janet_table_put(temptab, janet_ckeywordv("args"), janet_wrap_array(args));
    janet_gcroot(janet_wrap_table(temptab));

    /* Unlock GC */
    janet_gcunlock(handle);

    /* Run everything */
    JanetFiber *fiber = janet_fiber(jfunc, 64, argc, argc ? args->data : NULL);
    fiber->env = temptab;
    Janet out;
    JanetSignal result = janet_continue(fiber, janet_wrap_nil(), &out);
    if (result != JANET_SIGNAL_OK && result != JANET_SIGNAL_EVENT) {
      janet_stacktrace(fiber, out);
      janet_deinit();
      return result;
    }
#ifdef JANET_NET
    janet_loop();
#endif
    janet_deinit();
    return 0;
}
