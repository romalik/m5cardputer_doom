/*
 * GCC-PLUGINS - A collection of simple GCC plugins
 * Copyright (C) 2011 Daniel Marjamäki
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/**
 * Utility functions for parsing a tree
 * is_tree_code : match TREE_CODE for a tree node
 * is_declaration : is given tree node a PARM_DECL or VAR_DECL?
 * is_value : is given tree node a INTEGER_CST with given value
 * parse_tree : recursively parse the given tree
 */

#include "tree.h"
#include "cp/cp-tree.h"
#include "tree-iterator.h"

/** Match tree_code for the given "tree" */
static int is_tree_code(tree t, enum tree_code tc)
{
    return (t && (TREE_CODE(t) == tc)) ? 1 : 0;
}

/** Is the given "tree" a PARM_DECL or VAR_DECL? */
static int is_declaration(tree t)
{
    return (is_tree_code(t, PARM_DECL) || is_tree_code(t, VAR_DECL)) ? 1 : 0;
}

/** Is the given "tree" a INTEGER_CST with the given value? */
static int is_value(tree t, int value)
{
    return (is_tree_code(t, INTEGER_CST) && TREE_INT_CST_HIGH(t) == 0 && TREE_INT_CST_LOW(t) == value) ? 1 : 0;
}

/**
 * Parse given tree recursively and call callback for each tree node
 */
static void parse_tree(tree t, void (*callback)(tree t, int indent), int indent)
{
    // null => return
    if (t == 0)
        return;

    (*callback)(t, indent);

    // Statement list..
    if (TREE_CODE(t) == STATEMENT_LIST) {
        tree_stmt_iterator it;
        for (it = tsi_start(t); !tsi_end_p(it); tsi_next(&it)) {
            parse_tree(tsi_stmt(it), callback, indent+1);
        }
        return;
    }

    // Don't parse into declarations/exceptions/constants..
    if (DECL_P(t) || EXCEPTIONAL_CLASS_P(t) || CONSTANT_CLASS_P(t)) {
        return;
    }

    // parse into first operand
    parse_tree(TREE_OPERAND(t, 0), callback, indent+1);

    if (UNARY_CLASS_P(t))
        return;

    // parse into second operand
    enum tree_code code = TREE_CODE(t);
    if (code != RETURN_EXPR && 
        code != LABEL_EXPR &&
        code != GOTO_EXPR &&
        code != NOP_EXPR &&
        code != DECL_EXPR &&
        code != ADDR_EXPR && 
        code != INDIRECT_REF &&
        code != COMPONENT_REF)
        parse_tree(TREE_OPERAND(t, 1), callback, indent+1);
}
