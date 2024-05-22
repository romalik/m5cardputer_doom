#include <iostream>

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include "tree.h"
#include "gimple.h"
#include "plugin-version.h"
#include "tree-pass.h"
#include "context.h"
#include "function.h"

#include "gimple-iterator.h"
#include "gimple-pretty-print.h"
#include "ssa.h"
#include "tree-ssanames.h"
// We must assert that this plugin is GPL compatible
int plugin_is_GPL_compatible;

static struct plugin_info my_gcc_plugin_info = { "1.0", "This is a very simple plugin" };



namespace
{
    const pass_data my_first_pass_data = 
    {
        GIMPLE_PASS,
        "my_first_pass",        /* name */
        OPTGROUP_NONE,          /* optinfo_flags */
        TV_NONE,                /* tv_id */
        PROP_gimple_any,        /* properties_required */
        0,                      /* properties_provided */
        0,                      /* properties_destroyed */
        0,                      /* todo_flags_start */
        0                       /* todo_flags_finish */
    };


    struct my_first_pass : gimple_opt_pass
    {
        my_first_pass(gcc::context *ctx)
            : gimple_opt_pass(my_first_pass_data, ctx)
        {
        }

        virtual unsigned int execute(function* fun) override
        {
            // fun is the current function being called
            gimple_seq gimple_body = fun->gimple_body;

            std::cerr << "FUNCTION '" << function_name(fun)
                << "' at " << LOCATION_FILE(fun->function_start_locus) << ":" << LOCATION_LINE(fun->function_start_locus) << "\n";
            std::cerr << "*******************\n";

            // Dump its body
            print_gimple_seq(stderr, gimple_body, 0, static_cast<dump_flags_t>(0));
            std::cerr << "*******************\n";
            
            std::cerr << "\n\nProcess:\n";

            {
                FILE *file = stderr;
                gimple_seq seq = gimple_body;
                int spc = 3;
                dump_flags_t flags = static_cast<dump_flags_t>(0);


                pretty_printer buffer;
                pp_needs_newline (&buffer) = true;
                buffer.buffer->stream = file;

                gimple_stmt_iterator i;

                for (i = gsi_start (seq); !gsi_end_p (i); gsi_next (&i)){
                    gimple *gs = gsi_stmt (i);


                    if(gimple_code (gs) == GIMPLE_CALL) {
                        //std::cerr << "call:\n";
                    }


                    for(int ii = 0; ii < spc; ii++) {
                        //std::cerr << " ";
                    }


                    pp_gimple_stmt_1 (&buffer, gs, spc, flags);
                    pp_newline_and_flush (&buffer);

                    if(gimple_code (gs) == GIMPLE_ASSIGN) {
                        tree arg1 = NULL;
                        tree arg2 = NULL;
                        tree arg3 = NULL;
                        switch (gimple_num_ops (gs))
                        {
                        case 4:
                        arg3 = gimple_assign_rhs3 (gs);
                        /* FALLTHRU */
                        case 3:
                        arg2 = gimple_assign_rhs2 (gs);
                        /* FALLTHRU */
                        case 2:
                        arg1 = gimple_assign_rhs1 (gs);
                        break;
                        default:
                        gcc_unreachable ();
                        }

                        if(gimple_num_ops (gs) == 2) { 
                            enum tree_code rhs_code = gimple_assign_rhs_code (gs);
                            tree ptr_val = TREE_OPERAND (arg1, 0);
                            if(TREE_CODE_CLASS (rhs_code) == tcc_reference) {
                                
                                
                                //gs->code = GIMPLE_CALL;


                                std::cerr << "\n************************* This is a reference!\n\n";
                                tree function_fn;
                                tree function_fn_type;

                                function_fn_type = build_function_type_list(ptr_type_node, ptr_type_node, NULL_TREE);
                                function_fn = build_fn_decl ("__remap_ptr", function_fn_type);

                                tree ptr = arg1;// gimple_call_arg (gs, 1);


                                tree tmp_ptr = make_ssa_name (ptr_type_node);
                                gimple * call = gimple_build_call (function_fn, 1, ptr_val);
                                gimple_call_set_lhs(call, tmp_ptr);
                                //gimple *g = gimple_build_assign (make_ssa_name (pointer_sized_int_node), gimple_call_lhs(call));
                                //gsi_insert_before (&i, g, GSI_NEW_STMT);
                                

                                //gimple * new_assign = gimple_build_assign(gimple_assign_lhs (gs), rhs_code, tmp_ptr);
                                {
                                    std::cerr << "       ins:  ";
                                    pp_gimple_stmt_1 (&buffer, call, spc, flags);
                                    pp_newline_and_flush (&buffer);
                                }

                                gsi_insert_before (&i, call, GSI_NEW_STMT);
                                gsi_next (&i);
                                //gsi_insert_before (&i, new_assign, GSI_NEW_STMT);
                                //gsi_next (&i);



                                TREE_OPERAND (*gimple_assign_rhs1_ptr (gs),0) = tmp_ptr;
                                //gsi_remove (&i, true);
                                {
                                    std::cerr << "   replace:  ";
                                    pp_gimple_stmt_1 (&buffer, gs, spc, flags);
                                    pp_newline_and_flush (&buffer);
                                }


                            }
                        }
                    }

                }
            }


            std::cerr << "\n\nAfter replace:\n\n";

            print_gimple_seq(stderr, gimple_body, 0, static_cast<dump_flags_t>(0));
            std::cerr << "*******************\n\n";

            return 0;
        }

        virtual my_first_pass* clone() override
        {
            // We do not clone ourselves
            return this;
        }
    };
}

int plugin_init (struct plugin_name_args *plugin_info,
		struct plugin_gcc_version *version)
{
	// We check the current gcc loading this plugin against the gcc we used to
	// created this plugin
    std::cerr << "plugin version: " << version->basever << " gcc_version " << gcc_version.basever << "\n";
	if (strncmp (version->basever, gcc_version.basever, 3))
    {
        std::cerr << "This GCC plugin is for version " << GCCPLUGIN_VERSION_MAJOR << "." << GCCPLUGIN_VERSION_MINOR << "\n";
		return 1;
    }


    register_callback(plugin_info->base_name,
            /* event */ PLUGIN_INFO,
            /* callback */ NULL, /* user_data */ &my_gcc_plugin_info);

    // Register the phase right after omplower
    struct register_pass_info pass_info;

    // Note that after the cfg is built, fun->gimple_body is not accessible
    // anymore so we run this pass just before it
    pass_info.pass = new my_first_pass(g);
    pass_info.reference_pass_name = "cfg";
    pass_info.ref_pass_instance_number = 1;
    pass_info.pos_op = PASS_POS_INSERT_BEFORE;

    register_callback (plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);

    return 0;
}
