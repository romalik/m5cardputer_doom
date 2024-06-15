#include <iostream>
#include <vector>
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
#include "gimple-walk.h"
#include "gimplify.h"

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "gimple.h"
#include "cfghooks.h"
#include "alloc-pool.h"
#include "tree-pass.h"
#include "memmodel.h"
#include "tm_p.h"
#include "ssa.h"
#include "stringpool.h"
#include "tree-ssanames.h"

#include "emit-rtl.h"
#include "cgraph.h"
#include "gimple-pretty-print.h"
#include "alias.h"
#include "fold-const.h"
#include "cfganal.h"
#include "gimplify.h"
#include "gimple-iterator.h"
#include "varasm.h"
#include "stor-layout.h"
#include "tree-iterator.h"
#include "stringpool.h"
#include "attribs.h"

#include "dojump.h"
#include "explow.h"
#include "expr.h"
#include "output.h"
#include "langhooks.h"
#include "cfgloop.h"
#include "gimple-builder.h"
#include "gimple-fold.h"
#include "ubsan.h"
#include "params.h"
#include "builtins.h"
#include "fnmatch.h"
#include "tree-inline.h"
#include "convert.h"
#include "cp/cp-tree.h"

// We must assert that this plugin is GPL compatible
int plugin_is_GPL_compatible;

static struct plugin_info my_gcc_plugin_info = { "1.0", "This is a very simple plugin" };




/* BASE can already be an SSA_NAME; in that case, do not create a
   new SSA_NAME for it.  */

static tree
maybe_create_ssa_name(location_t loc, tree base, gimple_stmt_iterator *iter,
                      bool before_p)
{
    if (TREE_CODE(base) == SSA_NAME)
        return base;
    gimple *g = gimple_build_assign(make_ssa_name(TREE_TYPE(base)),
                                    TREE_CODE(base), base);
    gimple_set_location(g, loc);
    if (before_p)
        gsi_insert_before(iter, g, GSI_SAME_STMT);
    else
        gsi_insert_after(iter, g, GSI_NEW_STMT);
    return gimple_assign_lhs(g);
}

/* LEN can already have necessary size and precision;
   in that case, do not create a new variable.  */

tree maybe_cast_to_ptrmode(location_t loc, tree len, gimple_stmt_iterator *iter,
                           bool before_p)
{
    if (ptrofftype_p(len))
        return len;
    gimple *g = gimple_build_assign(make_ssa_name(pointer_sized_int_node),
                                    NOP_EXPR, len);
    gimple_set_location(g, loc);
    if (before_p)
        gsi_insert_before(iter, g, GSI_SAME_STMT);
    else
        gsi_insert_after(iter, g, GSI_NEW_STMT);
    return gimple_assign_lhs(g);
}

/* Instrument the memory access instruction BASE.  Insert new
   statements before or after ITER.

   Note that the memory access represented by BASE can be either an
   SSA_NAME, or a non-SSA expression.  LOCATION is the source code
   location.  IS_STORE is TRUE for a store, FALSE for a load.
   BEFORE_P is TRUE for inserting the instrumentation code before
   ITER, FALSE for inserting it after ITER.  IS_SCALAR_ACCESS is TRUE
   for a scalar memory access and FALSE for memory region access.
   NON_ZERO_P is TRUE if memory region is guaranteed to have non-zero
   length.  ALIGN tells alignment of accessed memory object.

   START_INSTRUMENTED and END_INSTRUMENTED are TRUE if start/end of
   memory region have already been instrumented.

   If BEFORE_P is TRUE, *ITER is arranged to still point to the
   statement it was pointing to prior to calling this function,
   otherwise, it points to the statement logically following it.  */

static tree
build_check_stmt(location_t loc, tree base, tree len,
                 HOST_WIDE_INT size_in_bytes, gimple_stmt_iterator *iter,
                 bool is_non_zero_len, bool before_p, bool is_store,
                 bool is_scalar_access, unsigned int align = 0)
{
    gimple_stmt_iterator gsi = *iter;
    gimple *g;


    gcc_assert(!(size_in_bytes > 0 && !is_non_zero_len));

    gsi = *iter;

    base = unshare_expr(base);
    base = maybe_create_ssa_name(loc, base, &gsi, before_p);

    if (len)
    {
        len = unshare_expr(len);
        len = maybe_cast_to_ptrmode(loc, len, iter, before_p);
    }
    else
    {
        gcc_assert(size_in_bytes != -1);
        len = build_int_cst(pointer_sized_int_node, size_in_bytes);
    }

    if (size_in_bytes > 1)
    {
        if ((size_in_bytes & (size_in_bytes - 1)) != 0 || size_in_bytes > 16)
            is_scalar_access = false;
        else if (align && align < size_in_bytes * BITS_PER_UNIT)
        {
            /* On non-strict alignment targets, if
               16-byte access is just 8-byte aligned,
               this will result in misaligned shadow
               memory 2 byte load, but otherwise can
               be handled using one read.  */
            if (size_in_bytes != 16 || STRICT_ALIGNMENT || align < 8 * BITS_PER_UNIT)
                is_scalar_access = false;
        }
    }


    HOST_WIDE_INT is_store_flag = is_store;

    tree function_remap_type = build_function_type_list(ptr_type_node/*remapped_ptr*/, 
                                                        ptr_type_node/*ptr**/, 
                                                        integer_type_node/*is_store*/, 
                                                        integer_type_node/*align*/, 
                                                        NULL_TREE);

    tree function_remap = build_fn_decl("__remap_ptr", function_remap_type);

    g = gimple_build_call(function_remap, 4, base,
                          build_int_cst(integer_type_node, is_store_flag),
                          len,
                          build_int_cst(integer_type_node,
                                        align / BITS_PER_UNIT));


    tree tmp_ptr = make_ssa_name (ptr_type_node);
    
    //fprintf(stderr, "Tree code for tmp_ptr: %s\n", get_tree_code_name(TREE_CODE(tmp_ptr)));

    gimple_call_set_lhs(g, tmp_ptr);


    gimple_set_location(g, loc);
    if (before_p)
        gsi_insert_before(&gsi, g, GSI_SAME_STMT);
    else
    {
        gsi_insert_after(&gsi, g, GSI_NEW_STMT);
        gsi_next(&gsi);
        *iter = gsi;
    }
    return tmp_ptr;
}



/* If T represents a memory access, add instrumentation code before ITER.
   LOCATION is source code location.
   IS_STORE is either TRUE (for a store) or FALSE (for a load).  */

#define MY_INSTRUMENT_READS 1
#define MY_INSTRUMENT_WRITES 0

static void
instrument_derefs(gimple_stmt_iterator *iter, tree t,
                  location_t location, bool is_store, tree bitfield)
{



        if(bitfield) {
       
        /*
                fprintf(stderr, "Bitfield!\n");
                tree aaa = make_ssa_name (integer_type_node);
                gimple * ggg = gimple_build_assign(t,t);
                pretty_printer buffer;
                pp_needs_newline (&buffer) = true;
                buffer.buffer->stream = stderr;
                pp_gimple_stmt_1 (&buffer, ggg, 0, 0);
                pp_newline_and_flush (&buffer);
                */

        }



    if (is_store && !MY_INSTRUMENT_WRITES)
        return;
    if (!is_store && !MY_INSTRUMENT_READS)
        return;

    tree type, base;
    HOST_WIDE_INT size_in_bytes;
    if (location == UNKNOWN_LOCATION)
        location = EXPR_LOCATION(t);

    type = TREE_TYPE(t);
    switch (TREE_CODE(t))
    {
    case ARRAY_REF:
    case COMPONENT_REF:
    case INDIRECT_REF:
    case MEM_REF:
    case VAR_DECL:
    case BIT_FIELD_REF:
        break;
        /* FALLTHRU */
    default:
        return;
    }

    size_in_bytes = int_size_in_bytes(type);
    if (size_in_bytes <= 0)
        return;

    poly_int64 bitsize, bitpos;
    tree offset;
    machine_mode mode;
    int unsignedp, reversep, volatilep = 0;
    tree inner = get_inner_reference(t, &bitsize, &bitpos, &offset, &mode,
                                     &unsignedp, &reversep, &volatilep);

    if (TREE_CODE(t) == COMPONENT_REF && DECL_BIT_FIELD_REPRESENTATIVE(TREE_OPERAND(t, 1)) != NULL_TREE)
    {
        /*
        ***********************************8
        possible ways to solve:

         - get_bit_range

         - val = convert_to_integer_nofold (TYPE_MAIN_VARIANT (bitfield_type), val);
        */


        tree repr = DECL_BIT_FIELD_REPRESENTATIVE(TREE_OPERAND(t, 1));
        //fprintf(stderr, "repr size in bytes: %lu\n",int_size_in_bytes(TREE_TYPE(repr)));
        instrument_derefs(iter, build3(COMPONENT_REF, TREE_TYPE(repr), TREE_OPERAND(t, 0), repr, TREE_OPERAND(t, 2)),
                          location, is_store, t);
        return;
    }

    if (!multiple_p(bitpos, BITS_PER_UNIT) || maybe_ne(bitsize, size_in_bytes * BITS_PER_UNIT))
        return;

    if (VAR_P(inner) && DECL_HARD_REGISTER(inner))
        return;

    poly_int64 decl_size;
    if (VAR_P(inner) && offset == NULL_TREE && DECL_SIZE(inner) && poly_int_tree_p(DECL_SIZE(inner), &decl_size) && known_subrange_p(bitpos, bitsize, 0, decl_size))
    {
        if (DECL_THREAD_LOCAL_P(inner))
            return;
        if (is_global_var(inner))
            return;
        if (!TREE_STATIC(inner))
        {
            /* Automatic vars in the current function will be always
               accessible.  */
            if (decl_function_context(inner) == current_function_decl && (!TREE_ADDRESSABLE(inner)))
                return;
        }
        /* Always instrument external vars, they might be dynamically
       initialized.  */
        else if (!DECL_EXTERNAL(inner))
        {
            /* For static vars if they are known not to be dynamically
               initialized, they will be always accessible.  */
            varpool_node *vnode = varpool_node::get(inner);
            if (vnode && !vnode->dynamically_initialized)
                return;
        }
    }

    base = build_fold_addr_expr(t);
    if (1)
    {
        unsigned int align = get_object_alignment(t);
        tree new_ptr = build_check_stmt(location, base, NULL_TREE, size_in_bytes, iter,
                                        /*is_non_zero_len*/ size_in_bytes > 0, /*before_p=*/true,
                                        is_store, /*is_scalar_access*/ true, align);


        //gimple * cast_type = build_type_cast(build_pointer_type(TREE_TYPE(gimple_assign_lhs(gsi_stmt(*iter)))), new_ptr);






        if (!bitfield)
        {
            gimple * cast_type = build_type_cast(build_pointer_type(TREE_TYPE(gimple_assign_lhs(gsi_stmt(*iter)))), new_ptr);
            gimple * new_assign = gimple_build_assign(  gimple_assign_lhs(gsi_stmt(*iter)),  
                                                        build_simple_mem_ref(  
                                                            //build_int_cst(integer_type_node, 1234)
                                                            gimple_assign_lhs(cast_type)
                                                        )
                                                    );

            gsi_insert_before(iter, cast_type, GSI_SAME_STMT);
            gsi_insert_before(iter, new_assign, GSI_SAME_STMT);


        } else {

            machine_mode mode1;
            poly_int64 bitsize, bitpos;
            poly_uint64 bitregion_start = 0;
            poly_uint64 bitregion_end = 0;
            tree offset;
            int unsignedp, reversep, volatilep = 0;


            /*tem = */get_inner_reference(bitfield, &bitsize, &bitpos, &offset, &mode1,
                                      &unsignedp, &reversep, &volatilep);

            if (maybe_lt(bitpos, 0))
            {
                gcc_assert(offset == NULL_TREE);
                offset = size_int(bits_to_bytes_round_down(bitpos));
                bitpos = num_trailing_bits(bitpos);
            }
            if (TREE_CODE(bitfield) == COMPONENT_REF && DECL_BIT_FIELD_TYPE(TREE_OPERAND(bitfield, 1)))
                get_bit_range(&bitregion_start, &bitregion_end, bitfield, &bitpos, &offset);
            /* The C++ memory model naturally applies to byte-aligned fields.
           However, if we do not have a DECL_BIT_FIELD_TYPE but BITPOS or
           BITSIZE are not byte-aligned, there is no need to limit the range
           we can access.  This can occur with packed structures in Ada.  */
            else if (maybe_gt(bitsize, 0) && multiple_p(bitsize, BITS_PER_UNIT) && multiple_p(bitpos, BITS_PER_UNIT))
            {
                bitregion_start = bitpos;
                bitregion_end = bitpos + bitsize - 1;
            }

            //fprintf(stderr, "bitregion start %lu end %lu bitpos %ld bitsize %ld\n", bitregion_start.to_constant(), bitregion_end.to_constant(), bitpos.to_constant(), bitsize.to_constant());


            tree derefed_value = make_ssa_name (integer_type_node);
            gimple * cast_type_repr = build_type_cast(build_pointer_type(integer_type_node), new_ptr);
            gimple * bitfield_deref_repr = gimple_build_assign      (   derefed_value,
                                                                        build_simple_mem_ref(  
                                                                            gimple_assign_lhs(cast_type_repr)
                                                                        )
                                                                    );

            tree lshifted_value = make_ssa_name (integer_type_node);

            int bitpos_in_region = bitpos.to_constant() - bitregion_start.to_constant();
            

            int shift_to_msb = (int_size_in_bytes(TREE_TYPE(lshifted_value))*8) - (bitpos_in_region + bitsize.to_constant());

            //fprintf(stderr, "target size_in_bytes %lu %lubits\n", int_size_in_bytes(TREE_TYPE(lshifted_value)), int_size_in_bytes(TREE_TYPE(lshifted_value))*8);
            //fprintf(stderr, "shift_to_msb %d\n", shift_to_msb);

            gimple * bitfield_adjust_lshift = gimple_build_assign   (   lshifted_value,
                                                                        LSHIFT_EXPR,  
                                                                        derefed_value,
                                                                        build_int_cst(
                                                                            integer_type_node, 
                                                                            shift_to_msb
                                                                        )
                                                                    );

            tree rshifted_value = make_ssa_name (integer_type_node);


            int shift_to_lsb = shift_to_msb + bitpos_in_region;
            //fprintf(stderr, "shift_to_lsb %d\n", shift_to_lsb);

            gimple * bitfield_adjust_rshift  = gimple_build_assign  (   rshifted_value,
                                                                        RSHIFT_EXPR,  
                                                                        lshifted_value,
                                                                        build_int_cst(
                                                                            integer_type_node,
                                                                            shift_to_lsb
                                                                        )
                                                                    );

            gimple * cast_type = build_type_cast(type, rshifted_value);

            gimple * new_assign = gimple_build_assign(  gimple_assign_lhs(gsi_stmt(*iter)),  
                                                        gimple_assign_lhs(cast_type)
                                                    );



            gsi_insert_before(iter, cast_type_repr, GSI_SAME_STMT);
            gsi_insert_before(iter, bitfield_deref_repr, GSI_SAME_STMT);
            gsi_insert_before(iter, bitfield_adjust_lshift, GSI_SAME_STMT);
            gsi_insert_before(iter, bitfield_adjust_rshift, GSI_SAME_STMT);
            gsi_insert_before(iter, cast_type, GSI_SAME_STMT);
            gsi_insert_before(iter, new_assign, GSI_SAME_STMT);

        }

        
        gsi_remove(iter, true);
        gsi_prev(iter);
    }
}

/*  Insert a memory reference into the hash table if access length
    can be determined in compile time.  */
#if 0
static void
maybe_update_mem_ref_hash_table (tree base, tree len)
{
  if (!POINTER_TYPE_P (TREE_TYPE (base))
      || !INTEGRAL_TYPE_P (TREE_TYPE (len)))
    return;

  HOST_WIDE_INT size_in_bytes = tree_fits_shwi_p (len) ? tree_to_shwi (len) : -1;

  if (size_in_bytes != -1)
    update_mem_ref_hash_table (base, size_in_bytes);
}

/* Instrument an access to a contiguous memory region that starts at
   the address pointed to by BASE, over a length of LEN (expressed in
   the sizeof (*BASE) bytes).  ITER points to the instruction before
   which the instrumentation instructions must be inserted.  LOCATION
   is the source location that the instrumentation instructions must
   have.  If IS_STORE is true, then the memory access is a store;
   otherwise, it's a load.  */

static void
instrument_mem_region_access (tree base, tree len,
			      gimple_stmt_iterator *iter,
			      location_t location, bool is_store)
{
  if (!POINTER_TYPE_P (TREE_TYPE (base))
      || !INTEGRAL_TYPE_P (TREE_TYPE (len))
      || integer_zerop (len))
    return;

  HOST_WIDE_INT size_in_bytes = tree_fits_shwi_p (len) ? tree_to_shwi (len) : -1;

  if ((size_in_bytes == -1)
      || !has_mem_ref_been_instrumented (base, size_in_bytes))
    {
      build_check_stmt (location, base, len, size_in_bytes, iter,
			/*is_non_zero_len*/size_in_bytes > 0, /*before_p*/true,
			is_store, /*is_scalar_access*/false, /*align*/0);
    }

  maybe_update_mem_ref_hash_table (base, len);
  *iter = gsi_for_stmt (gsi_stmt (*iter));
}

/* Instrument the call to a built-in memory access function that is
   pointed to by the iterator ITER.

   Upon completion, return TRUE iff *ITER has been advanced to the
   statement following the one it was originally pointing to.  */

static bool
instrument_builtin_call (gimple_stmt_iterator *iter)
{
  if (!ASAN_MEMINTRIN)
    return false;

  bool iter_advanced_p = false;
  gcall *call = as_a <gcall *> (gsi_stmt (*iter));

  gcc_checking_assert (gimple_call_builtin_p (call, BUILT_IN_NORMAL));

  location_t loc = gimple_location (call);

  asan_mem_ref src0, src1, dest;
  asan_mem_ref_init (&src0, NULL, 1);
  asan_mem_ref_init (&src1, NULL, 1);
  asan_mem_ref_init (&dest, NULL, 1);

  tree src0_len = NULL_TREE, src1_len = NULL_TREE, dest_len = NULL_TREE;
  bool src0_is_store = false, src1_is_store = false, dest_is_store = false,
    dest_is_deref = false, intercepted_p = true;

  if (get_mem_refs_of_builtin_call (call,
				    &src0, &src0_len, &src0_is_store,
				    &src1, &src1_len, &src1_is_store,
				    &dest, &dest_len, &dest_is_store,
				    &dest_is_deref, &intercepted_p, iter))
    {
      if (dest_is_deref)
	{
	  instrument_derefs (iter, dest.start, loc, dest_is_store);
	  gsi_next (iter);
	  iter_advanced_p = true;
	}
      else if (!intercepted_p
	       && (src0_len || src1_len || dest_len))
	{
	  if (src0.start != NULL_TREE)
	    instrument_mem_region_access (src0.start, src0_len,
					  iter, loc, /*is_store=*/false);
	  if (src1.start != NULL_TREE)
	    instrument_mem_region_access (src1.start, src1_len,
					  iter, loc, /*is_store=*/false);
	  if (dest.start != NULL_TREE)
	    instrument_mem_region_access (dest.start, dest_len,
					  iter, loc, /*is_store=*/true);

	  *iter = gsi_for_stmt (call);
	  gsi_next (iter);
	  iter_advanced_p = true;
	}
      else
	{
	  if (src0.start != NULL_TREE)
	    maybe_update_mem_ref_hash_table (src0.start, src0_len);
	  if (src1.start != NULL_TREE)
	    maybe_update_mem_ref_hash_table (src1.start, src1_len);
	  if (dest.start != NULL_TREE)
	    maybe_update_mem_ref_hash_table (dest.start, dest_len);
	}
    }
  return iter_advanced_p;
}
#endif
/*  Instrument the assignment statement ITER if it is subject to
    instrumentation.  Return TRUE iff instrumentation actually
    happened.  In that case, the iterator ITER is advanced to the next
    logical expression following the one initially pointed to by ITER,
    and the relevant memory reference that which access has been
    instrumented is added to the memory references hash table.  */

static bool
maybe_instrument_assignment (gimple_stmt_iterator *iter)
{
    gimple *s = gsi_stmt (*iter);

    //gcc_assert (gimple_assign_single_p (s));

    tree ref_expr = NULL_TREE;
    bool is_store, is_instrumented = false;

    //fprintf(stderr, "maybe_instrument_assignment\n");
            if(0){
                pretty_printer buffer;
                pp_needs_newline (&buffer) = true;
                buffer.buffer->stream = stderr;
                pp_gimple_stmt_1 (&buffer, s, 0, 0);
                pp_newline_and_flush (&buffer);                
            }

    if (gimple_store_p (s)) {
        ref_expr = gimple_assign_lhs (s);
        is_store = true;
        instrument_derefs (iter, ref_expr,
                gimple_location (s),
                is_store, NULL_TREE);
        is_instrumented = true;
    }

    if (gimple_assign_load_p (s))
    {
        //fprintf(stderr, "Have load!\n");
        ref_expr = gimple_assign_rhs1 (s);
        is_store = false;
        instrument_derefs (iter, ref_expr,
                gimple_location (s),
                is_store, NULL_TREE);
        is_instrumented = true;
    }

    //fprintf(stderr, "maybe_instrument_assignment done\n");
/*
    if (is_instrumented)
        gsi_next (iter);
*/
  return is_instrumented;
}

/* Instrument the function call pointed to by the iterator ITER, if it
   is subject to instrumentation.  At the moment, the only function
   calls that are instrumented are some built-in functions that access
   memory.  Look at instrument_builtin_call to learn more.

   Upon completion return TRUE iff *ITER was advanced to the statement
   following the one it was originally pointing to.  */
#if 0
static bool
maybe_instrument_call (gimple_stmt_iterator *iter)
{
  gimple *stmt = gsi_stmt (*iter);
  bool is_builtin = gimple_call_builtin_p (stmt, BUILT_IN_NORMAL);

  if (is_builtin && instrument_builtin_call (iter))
    return true;

  if (gimple_call_noreturn_p (stmt))
    {
      if (is_builtin)
	{
	  tree callee = gimple_call_fndecl (stmt);
	  switch (DECL_FUNCTION_CODE (callee))
	    {
	    case BUILT_IN_UNREACHABLE:
	    case BUILT_IN_TRAP:
	      /* Don't instrument these.  */
	      return false;
	    default:
	      break;
	    }
	}
      tree decl = builtin_decl_implicit (BUILT_IN_ASAN_HANDLE_NO_RETURN);
      gimple *g = gimple_build_call (decl, 0);
      gimple_set_location (g, gimple_location (stmt));
      gsi_insert_before (iter, g, GSI_SAME_STMT);
    }

  bool instrumented = false;
  if (gimple_store_p (stmt))
    {
      tree ref_expr = gimple_call_lhs (stmt);
      instrument_derefs (iter, ref_expr,
			 gimple_location (stmt),
			 /*is_store=*/true);

      instrumented = true;
    }

  /* Walk through gimple_call arguments and check them id needed.  */
  unsigned args_num = gimple_call_num_args (stmt);
  for (unsigned i = 0; i < args_num; ++i)
    {
      tree arg = gimple_call_arg (stmt, i);
      /* If ARG is not a non-aggregate register variable, compiler in general
	 creates temporary for it and pass it as argument to gimple call.
	 But in some cases, e.g. when we pass by value a small structure that
	 fits to register, compiler can avoid extra overhead by pulling out
	 these temporaries.  In this case, we should check the argument.  */
      if (!is_gimple_reg (arg) && !is_gimple_min_invariant (arg))
	{
	  instrument_derefs (iter, arg,
			     gimple_location (stmt),
			     /*is_store=*/false);
	  instrumented = true;
	}
    }
  if (instrumented)
    gsi_next (iter);
  return instrumented;
}
#endif






std::vector<gimple *> calls_to_add;

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

#if 0

        static tree tree_walk_cb (tree * tp, int *walk_subtrees, void *) {
            enum tree_code code = TREE_CODE(*tp);

            fprintf(stderr, "   Operand: %s\n", get_tree_code_name(code));
            //if(TREE_CODE_CLASS(code) == tcc_reference) {
            if(code == MEM_REF || code == ARRAY_REF) {
                std::cerr << "^^^^^^^^^^^^^^^^^^^^^^^^^^^ This is a reference!\n\n";
                tree ptr_val = TREE_OPERAND (*tp, 0);
                if(code == ARRAY_REF) {
                    ptr_val = TREE_OPERAND (*tp, 1);
                }

                tree tmp_param = make_ssa_name(ptr_type_node);
                gimple * assign_remap_parameter = gimple_build_assign(tmp_param, ptr_val);


                tree function_fn;
                tree function_fn_type;

                function_fn_type = build_function_type_list(ptr_type_node, ptr_type_node, NULL_TREE);
                function_fn = build_fn_decl ("__remap_ptr", function_fn_type);

                tree tmp_ptr = make_ssa_name (ptr_type_node);
                gimple * call = gimple_build_call (function_fn, 1, tmp_param);
                gimple_call_set_lhs(call, tmp_ptr);

                TREE_OPERAND (*tp,0) = tmp_ptr;

                calls_to_add.push_back(assign_remap_parameter);
                calls_to_add.push_back(call);
            }

            return NULL_TREE;
        }


        virtual unsigned int execute___(function* fun) override
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

                    //pp_gimple_stmt_1 (&buffer, gs, spc, flags);
                    //pp_newline_and_flush (&buffer);

                    walk_gimple_op(gs, my_first_pass::tree_walk_cb, NULL);

                    for(auto call: calls_to_add) {
                        gsi_insert_before (&i, call, GSI_NEW_STMT);
                        gsi_next (&i);
                    }
                    calls_to_add.clear();

                }


                #if 0
                if(0){

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
                                fprintf(stderr, "   operand: %s\n", get_tree_code_name(rhs_code));
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
                #endif
            }


            std::cerr << "\n\nAfter replace:\n\n";

            print_gimple_seq(stderr, gimple_body, 0, static_cast<dump_flags_t>(0));
            std::cerr << "*******************\n\n";

            return 0;
        }
#endif
        virtual unsigned int execute(function* fun) override
        {
            // fun is the current function being called
            gimple_seq gimple_body = fun->gimple_body;

            //fprintf(stderr, "enter function %s\n", function_name(fun));
            if(lookup_attribute ("no_instrument_function", DECL_ATTRIBUTES (fun->decl)) != NULL_TREE) {
                //fprintf(stderr, "Has attribute no_instrument_function\n");
                return 0;
            } else {
                //fprintf(stderr, "No attribute\n");
            }

/*
            std::cerr << "FUNCTION '" << function_name(fun)
                << "' at " << LOCATION_FILE(fun->function_start_locus) << ":" << LOCATION_LINE(fun->function_start_locus) << "\n";
            std::cerr << "*******************\n";

            // Dump its body
            print_gimple_seq(stderr, gimple_body, 0, static_cast<dump_flags_t>(0));
            std::cerr << "*******************\n";
            
            std::cerr << "\n\nProcess:\n";
*/
            {
                FILE *file = stderr;
                gimple_seq seq = gimple_body;
                //int spc = 3;
                //dump_flags_t flags = static_cast<dump_flags_t>(0);


                pretty_printer buffer;
                pp_needs_newline (&buffer) = true;
                buffer.buffer->stream = file;

                gimple_stmt_iterator i;

                for (i = gsi_start (seq); !gsi_end_p (i);gsi_next (&i)){
                    gimple *s = gsi_stmt (i);
                    if(is_gimple_assign(s)) {
                        maybe_instrument_assignment(&i);
                    }

#if 0
                    if (/*gimple_assign_single_p (s) &&*/ is_gimple_assign (s) && maybe_instrument_assignment (&i)) {
                        /*  Nothing to do as maybe_instrument_assignment advanced
                        the iterator I.  */
/*                        
                    } else if (is_gimple_call (s) && maybe_instrument_call (&i)) {
*/                        
                        /*  Nothing to do as maybe_instrument_call
                        advanced the iterator I.  */
                    }else {
                        /* No instrumentation happened.*/
                        
                    }
#endif

                }
            }

/*
            std::cerr << "\n\nAfter replace:\n\n";

            print_gimple_seq(stderr, fun->gimple_body, 0, static_cast<dump_flags_t>(0));
            std::cerr << "*******************\n\n";
*/
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
    //std::cerr << "plugin version: " << version->basever << " gcc_version " << gcc_version.basever << "\n";
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
