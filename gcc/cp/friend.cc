/* Help friends in C++.
   Copyright (C) 1997-2025 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "cp-tree.h"

/* Friend data structures are described in cp-tree.h.  */


/* The GLOBAL_FRIEND scope (functions, classes, or templates) is
   regarded as a friend of every class.  This is only used by libcc1,
   to enable GDB's code snippets to access private members without
   disabling access control in general, which could cause different
   template overload resolution results when accessibility matters
   (e.g. tests for an accessible member).  */

static GTY(()) tree global_friend;

/* Set the GLOBAL_FRIEND for this compilation session.  It might be
   set multiple times, but always to the same scope.  */

void
set_global_friend (tree scope)
{
  gcc_checking_assert (scope != NULL_TREE);
  gcc_assert (!global_friend || global_friend == scope);
  global_friend = scope;
}

/* Return TRUE if SCOPE is the global friend.  */

bool
is_global_friend (tree scope)
{
  gcc_checking_assert (scope != NULL_TREE);

  if (global_friend == scope)
    return true;

  if (!global_friend)
    return false;

  if (is_specialization_of_friend (global_friend, scope))
    return true;

  return false;
}

/* Returns nonzero if SUPPLICANT is a friend of TYPE.  */

int
is_friend (tree type, tree supplicant)
{
  int declp;
  tree list;
  tree context;

  if (supplicant == NULL_TREE || type == NULL_TREE)
    return 0;

  if (is_global_friend (supplicant))
    return 1;

  declp = DECL_P (supplicant);

  if (declp)
    /* It's a function decl.  */
    {
      tree list = DECL_FRIENDLIST (TYPE_MAIN_DECL (type));
      tree name = DECL_NAME (supplicant);

      for (; list ; list = TREE_CHAIN (list))
	{
	  if (name == FRIEND_NAME (list))
	    {
	      tree friends = FRIEND_DECLS (list);
	      for (; friends ; friends = TREE_CHAIN (friends))
		{
		  tree this_friend = TREE_VALUE (friends);

		  if (this_friend == NULL_TREE)
		    continue;

		  if (supplicant == this_friend)
		    return 1;

		  if (is_specialization_of_friend (supplicant, this_friend))
		    return 1;
		}
	      break;
	    }
	}
    }
  else
    /* It's a type.  */
    {
      if (same_type_p (supplicant, type))
	return 1;

      list = CLASSTYPE_FRIEND_CLASSES (TREE_TYPE (TYPE_MAIN_DECL (type)));
      for (; list ; list = TREE_CHAIN (list))
	{
	  tree t = TREE_VALUE (list);

	  if (TREE_CODE (t) == TEMPLATE_DECL ?
	      is_specialization_of_friend (TYPE_MAIN_DECL (supplicant), t) :
	      same_type_p (supplicant, t))
	    return 1;
	}
    }

  if (declp)
    {
      if (DECL_FUNCTION_MEMBER_P (supplicant))
	context = DECL_CONTEXT (supplicant);
      else if (tree fc = DECL_FRIEND_CONTEXT (supplicant))
	context = fc;
      else
	context = NULL_TREE;
    }
  else
    {
      if (TYPE_CLASS_SCOPE_P (supplicant))
	/* Nested classes get the same access as their enclosing types, as
	   per DR 45 (this is a change from the standard).  */
	context = TYPE_CONTEXT (supplicant);
      else
	/* Local classes have the same access as the enclosing function.  */
	context = decl_function_context (TYPE_MAIN_DECL (supplicant));
    }

  /* A namespace is not friend to anybody.  */
  if (context && TREE_CODE (context) == NAMESPACE_DECL)
    context = NULL_TREE;

  if (context)
    return is_friend (type, context);

  return 0;
}

/* Add a new friend to the friends of the aggregate type TYPE.
   DECL is the FUNCTION_DECL of the friend being added.

   If COMPLAIN is true, warning about duplicate friend is issued.
   We want to have this diagnostics during parsing but not
   when a template is being instantiated.  */

void
add_friend (tree type, tree decl, bool complain)
{
  tree typedecl;
  tree list;
  tree name;
  tree ctx;

  if (decl == error_mark_node)
    return;

  typedecl = TYPE_MAIN_DECL (type);
  list = DECL_FRIENDLIST (typedecl);
  name = DECL_NAME (decl);
  type = TREE_TYPE (typedecl);

  while (list)
    {
      if (name == FRIEND_NAME (list))
	{
	  tree friends = FRIEND_DECLS (list);
	  for (; friends ; friends = TREE_CHAIN (friends))
	    {
	      if (decl == TREE_VALUE (friends))
		{
		  if (complain)
		    warning (OPT_Wredundant_decls,
			     "%qD is already a friend of class %qT",
			     decl, type);
		  return;
		}
	    }

	  TREE_VALUE (list) = tree_cons (NULL_TREE, decl,
					 TREE_VALUE (list));
	  break;
	}
      list = TREE_CHAIN (list);
    }

  ctx = DECL_CONTEXT (decl);
  if (ctx && CLASS_TYPE_P (ctx) && !uses_template_parms (ctx))
    perform_or_defer_access_check (TYPE_BINFO (ctx), decl, decl,
				   tf_warning_or_error);

  maybe_add_class_template_decl_list (type, decl, /*friend_p=*/1);

  if (!list)
    DECL_FRIENDLIST (typedecl)
      = tree_cons (DECL_NAME (decl), build_tree_list (NULL_TREE, decl),
		   DECL_FRIENDLIST (typedecl));
  if (!uses_template_parms (type))
    DECL_BEFRIENDING_CLASSES (decl)
      = tree_cons (NULL_TREE, type,
		   DECL_BEFRIENDING_CLASSES (decl));
}

/* Make FRIEND_TYPE a friend class to TYPE.  If FRIEND_TYPE has already
   been defined, we make all of its member functions friends of
   TYPE.  If not, we make it a pending friend, which can later be added
   when its definition is seen.  If a type is defined, then its TYPE_DECL's
   DECL_UNDEFINED_FRIENDS contains a (possibly empty) list of friend
   classes that are not defined.  If a type has not yet been defined,
   then the DECL_WAITING_FRIENDS contains a list of types
   waiting to make it their friend.  Note that these two can both
   be in use at the same time!

   If COMPLAIN is true, warning about duplicate friend is issued.
   We want to have this diagnostics during parsing but not
   when a template is being instantiated.  */

void
make_friend_class (tree type, tree friend_type, bool complain)
{
  tree classes;

  /* CLASS_TEMPLATE_DEPTH counts the number of template headers for
     the enclosing class.  FRIEND_DEPTH counts the number of template
     headers used for this friend declaration.  TEMPLATE_MEMBER_P,
     defined inside the `if' block for TYPENAME_TYPE case, is true if
     a template header in FRIEND_DEPTH is intended for DECLARATOR.
     For example, the code

       template <class T> struct A {
	 template <class U> struct B {
	   template <class V> template <class W>
	     friend class C<V>::D;
	 };
       };

     will eventually give the following results

     1. CLASS_TEMPLATE_DEPTH equals 2 (for `T' and `U').
     2. FRIEND_DEPTH equals 2 (for `V' and `W').
     3. TEMPLATE_MEMBER_P is true (for `W').

     The friend is a template friend iff FRIEND_DEPTH is nonzero.  */

  int class_template_depth = template_class_depth (type);
  int friend_depth = 0;
  if (current_template_depth)
    /* When processing a friend declaration at parse time, just compare the
       current depth to that of the class template.  */
    friend_depth = current_template_depth - class_template_depth;
  else
    {
      /* Otherwise, we got here from instantiate_class_template.  Determine
	 the friend depth by looking at the template parameters used within
	 FRIEND_TYPE.  */
      gcc_checking_assert (class_template_depth == 0);
      while (uses_template_parms_level (friend_type, friend_depth + 1))
	++friend_depth;
    }

  if (! MAYBE_CLASS_TYPE_P (friend_type)
      && TREE_CODE (friend_type) != TEMPLATE_TEMPLATE_PARM
      && TREE_CODE (friend_type) != TYPE_PACK_EXPANSION)
    {
      /* N1791: If the type specifier in a friend declaration designates a
	 (possibly cv-qualified) class type, that class is declared as a
	 friend; otherwise, the friend declaration is ignored.

         So don't complain in C++11 mode.  */
      if (cxx_dialect < cxx11)
	pedwarn (input_location, complain ? 0 : OPT_Wpedantic,
		 "invalid type %qT declared %<friend%>", friend_type);
      return;
    }

  friend_type = cv_unqualified (friend_type);

  if (check_for_bare_parameter_packs (friend_type))
    return;

  if (friend_depth)
    {
      /* [temp.friend] Friend declarations shall not declare partial
	 specializations.  */
      if (CLASS_TYPE_P (friend_type)
	  && CLASSTYPE_TEMPLATE_SPECIALIZATION (friend_type)
	  && uses_template_parms (friend_type))
	{
	  error ("partial specialization %qT declared %<friend%>",
		 friend_type);
	  return;
	}

      if (TYPE_TEMPLATE_INFO (friend_type)
	  && !PRIMARY_TEMPLATE_P (TYPE_TI_TEMPLATE (friend_type)))
	{
	  auto_diagnostic_group d;
	  error ("%qT is not a template", friend_type);
	  inform (location_of (friend_type), "previous declaration here");
	  if (TYPE_CLASS_SCOPE_P (friend_type)
	      && CLASSTYPE_TEMPLATE_INFO (TYPE_CONTEXT (friend_type))
	      && currently_open_class (TYPE_CONTEXT (friend_type)))
	    inform (input_location, "perhaps you need explicit template "
		    "arguments in your nested-name-specifier");
	  return;
	}
    }

  /* It makes sense for a template class to be friends with itself,
     that means the instantiations can be friendly.  Other cases are
     not so meaningful.  */
  if (!friend_depth && same_type_p (type, friend_type))
    {
      if (complain)
	warning (0, "class %qT is implicitly friends with itself",
		 type);
      return;
    }

  /* [temp.friend]

     A friend of a class or class template can be a function or
     class template, a specialization of a function template or
     class template, or an ordinary (nontemplate) function or
     class.  */
  if (!friend_depth)
    ;/* ok */
  else if (TREE_CODE (friend_type) == TYPENAME_TYPE)
    {
      if (TREE_CODE (TYPENAME_TYPE_FULLNAME (friend_type))
	  == TEMPLATE_ID_EXPR)
	{
	  /* template <class U> friend class T::X<U>; */
	  /* [temp.friend]
	     Friend declarations shall not declare partial
	     specializations.  */
	  error ("partial specialization %qT declared %<friend%>",
		 friend_type);
	  return;
	}
      else
	{
	  /* We will figure this out later.  */
	  bool template_member_p = false;

	  tree ctype = TYPE_CONTEXT (friend_type);
	  tree name = TYPE_IDENTIFIER (friend_type);
	  tree decl;

	  /* We need to distinguish a TYPENAME_TYPE for the non-template
	     class B in
	       template<class T> friend class A<T>::B;
	     vs for the class template B in
	       template<class T> template<class U> friend class A<T>::B;  */
	  if (current_template_depth
	      && !uses_template_parms_level (ctype, current_template_depth))
	    template_member_p = true;

	  if (class_template_depth)
	    {
	      /* We rely on tsubst_friend_class to check the
		 validity of the declaration later.  */
	      if (template_member_p)
		friend_type
		  = make_unbound_class_template (ctype,
						 name,
						 current_template_parms,
						 tf_error);
	      else
		friend_type
		  = make_typename_type (ctype, name, class_type, tf_error);
	    }
	  else
	    {
	      decl = lookup_member (ctype, name, 0, true, tf_warning_or_error);
	      if (!decl)
		{
		  error ("%qT is not a member of %qT", name, ctype);
		  return;
		}
	      if (template_member_p && !DECL_CLASS_TEMPLATE_P (decl))
		{
		  auto_diagnostic_group d;
		  error ("%qT is not a member class template of %qT",
			 name, ctype);
		  inform (DECL_SOURCE_LOCATION (decl),
			  "%qD declared here", decl);
		  return;
		}
	      if (!template_member_p && (TREE_CODE (decl) != TYPE_DECL
					 || !CLASS_TYPE_P (TREE_TYPE (decl))))
		{
		  auto_diagnostic_group d;
		  error ("%qT is not a nested class of %qT",
			 name, ctype);
		  inform (DECL_SOURCE_LOCATION (decl),
			  "%qD declared here", decl);
		  return;
		}

	      friend_type = CLASSTYPE_TI_TEMPLATE (TREE_TYPE (decl));
	    }
	}
    }
  else if (TREE_CODE (friend_type) == TEMPLATE_TYPE_PARM)
    {
      /* template <class T> friend class T; */
      error ("template parameter type %qT declared %<friend%>", friend_type);
      return;
    }
  else if (TREE_CODE (friend_type) == TEMPLATE_TEMPLATE_PARM)
    friend_type = TYPE_NAME (friend_type);
  else if (!CLASSTYPE_TEMPLATE_INFO (friend_type))
    {
      /* template <class T> friend class A; where A is not a template */
      error ("%q#T is not a template", friend_type);
      return;
    }
  else
    /* template <class T> friend class A; where A is a template */
    friend_type = CLASSTYPE_TI_TEMPLATE (friend_type);

  if (friend_type == error_mark_node)
    return;

  /* See if it is already a friend.  */
  for (classes = CLASSTYPE_FRIEND_CLASSES (type);
       classes;
       classes = TREE_CHAIN (classes))
    {
      tree probe = TREE_VALUE (classes);

      if (TREE_CODE (friend_type) == TEMPLATE_DECL)
	{
	  if (friend_type == probe)
	    {
	      if (complain)
		warning (OPT_Wredundant_decls,
			 "%qD is already a friend of %qT", probe, type);
	      break;
	    }
	}
      else if (TREE_CODE (probe) != TEMPLATE_DECL)
	{
	  if (same_type_p (probe, friend_type))
	    {
	      if (complain)
		warning (OPT_Wredundant_decls,
			 "%qT is already a friend of %qT", probe, type);
	      break;
	    }
	}
    }

  if (!classes)
    {
      maybe_add_class_template_decl_list (type, friend_type, /*friend_p=*/1);

      CLASSTYPE_FRIEND_CLASSES (type)
	= tree_cons (NULL_TREE, friend_type, CLASSTYPE_FRIEND_CLASSES (type));
      if (TREE_CODE (friend_type) == TEMPLATE_DECL)
	friend_type = TREE_TYPE (friend_type);
      if (!uses_template_parms (type))
	CLASSTYPE_BEFRIENDING_CLASSES (friend_type)
	  = tree_cons (NULL_TREE, type,
		       CLASSTYPE_BEFRIENDING_CLASSES (friend_type));
    }
}

/* Record DECL (a FUNCTION_DECL) as a friend of the
   CURRENT_CLASS_TYPE.  If DECL is a member function, SCOPE is the
   class of which it is a member, as named in the friend declaration.
   If the friend declaration was explicitly namespace-qualified, SCOPE
   is that namespace.
   DECLARATOR is the name of the friend.  FUNCDEF_FLAG is true if the
   friend declaration is a definition of the function.  FLAGS is as
   for grokclass fn.  */

tree
do_friend (tree scope, tree declarator, tree decl,
	   enum overload_flags flags,
	   bool funcdef_flag)
{
  gcc_assert (TREE_CODE (decl) == FUNCTION_DECL);

  tree ctype = NULL_TREE;
  tree in_namespace = NULL_TREE;
  if (!scope)
    ;
  else if (MAYBE_CLASS_TYPE_P (scope))
    ctype = scope;
  else
    {
      gcc_checking_assert (TREE_CODE (scope) == NAMESPACE_DECL);
      in_namespace = scope;
    }

  /* Friend functions are unique, until proved otherwise.  */
  DECL_UNIQUE_FRIEND_P (decl) = 1;

  if (DECL_OVERRIDE_P (decl) || DECL_FINAL_P (decl))
    error ("friend declaration %qD may not have virt-specifiers",
	   decl);

  tree orig_declarator = declarator;
  if (TREE_CODE (declarator) == TEMPLATE_ID_EXPR)
    {
      declarator = TREE_OPERAND (declarator, 0);
      if (!identifier_p (declarator))
	declarator = OVL_NAME (declarator);
    }

  /* CLASS_TEMPLATE_DEPTH counts the number of template headers for
     the enclosing class.  FRIEND_DEPTH counts the number of template
     headers used for this friend declaration.  TEMPLATE_MEMBER_P is
     true if a template header in FRIEND_DEPTH is intended for
     DECLARATOR.  For example, the code

     template <class T> struct A {
       template <class U> struct B {
	 template <class V> template <class W>
	   friend void C<V>::f(W);
       };
     };

     will eventually give the following results

     1. CLASS_TEMPLATE_DEPTH equals 2 (for `T' and `U').
     2. FRIEND_DEPTH equals 2 (for `V' and `W').
     3. CTYPE_DEPTH equals 1 (for `V').
     4. TEMPLATE_MEMBER_P is true (for `W').  */

  int class_template_depth = template_class_depth (current_class_type);
  int friend_depth = current_template_depth - class_template_depth;
  int ctype_depth = num_template_headers_for_class (ctype);
  bool template_member_p = friend_depth > ctype_depth;

  if (ctype)
    {
      tree cname = TYPE_NAME (ctype);
      if (TREE_CODE (cname) == TYPE_DECL)
	cname = DECL_NAME (cname);

      /* A method friend.  */
      if (flags == NO_SPECIAL && declarator == cname)
	DECL_CXX_CONSTRUCTOR_P (decl) = 1;

      grokclassfn (ctype, decl, flags);

      /* A nested class may declare a member of an enclosing class
	 to be a friend, so we do lookup here even if CTYPE is in
	 the process of being defined.  */
      if (class_template_depth
	  || COMPLETE_OR_OPEN_TYPE_P (ctype))
	{
	  if (DECL_TEMPLATE_INFO (decl))
	    /* DECL is a template specialization.  No need to
	       build a new TEMPLATE_DECL.  */
	    ;
	  else if (class_template_depth)
	    /* We rely on tsubst_friend_function to check the
	       validity of the declaration later.  */
	    decl = push_template_decl (decl, /*is_friend=*/true);
	  else
	    decl = check_classfn (ctype, decl,
				  template_member_p
				  ? current_template_parms
				  : NULL_TREE);

	  if ((template_member_p
	       /* Always pull out the TEMPLATE_DECL if we have a friend
		  template in a class template so that it gets tsubsted
		  properly later on (59956).  tsubst_friend_function knows
		  how to tell this apart from a member template.  */
	       || (class_template_depth && friend_depth))
	      && decl && TREE_CODE (decl) == FUNCTION_DECL)
	    decl = DECL_TI_TEMPLATE (decl);
	}
      else
	error ("member %qD declared as friend before type %qT defined",
		  decl, ctype);
    }
  else
    {
      /* Namespace-scope friend function.  */

      if (funcdef_flag)
	SET_DECL_FRIEND_CONTEXT (decl, current_class_type);

      if (! DECL_USE_TEMPLATE (decl))
	{
	  /* We must check whether the decl refers to template
	     arguments before push_template_decl adds a reference to
	     the containing template class.  */
	  int warn = (warn_nontemplate_friend
		      && ! funcdef_flag && ! friend_depth
		      && current_template_parms
		      && uses_template_parms (decl));

	  if (friend_depth || class_template_depth)
	    /* We can't call pushdecl for a template class, since in
	       general, such a declaration depends on template
	       parameters.  Instead, we call pushdecl when the class
	       is instantiated.  */
	    decl = push_template_decl (decl, /*is_friend=*/true);
	  else if (current_function_decl && !in_namespace)
	    /* pushdecl will check there's a local decl already.  */
	    decl = pushdecl (decl, /*hiding=*/true);
	  else
	    {
	      /* We can't use pushdecl, as we might be in a template
		 class specialization, and pushdecl will insert an
		 unqualified friend decl into the template parameter
		 scope, rather than the namespace containing it.  */
	      tree ns = decl_namespace_context (decl);

	      push_nested_namespace (ns);
	      decl = pushdecl_namespace_level (decl, /*hiding=*/true);
	      pop_nested_namespace (ns);
	    }

	  if (warn)
	    {
	      static int explained;
	      bool warned;

	      auto_diagnostic_group d;
	      warned = warning (OPT_Wnon_template_friend, "friend declaration "
				"%q#D declares a non-template function", decl);
	      if (! explained && warned)
		{
		  inform (input_location, "(if this is not what you intended, "
			  "make sure the function template has already been "
			  "declared and add %<<>%> after the function name "
			  "here)");
		  explained = 1;
		}
	    }
	}
    }

  if (decl == error_mark_node)
    return error_mark_node;

  if (!class_template_depth && DECL_IMPLICIT_INSTANTIATION (decl)
      && TREE_CODE (DECL_TI_TEMPLATE (decl)) != TEMPLATE_DECL)
    /* "[if no non-template match is found,] each remaining function template
       is replaced with the specialization chosen by deduction from the
       friend declaration or discarded if deduction fails."

       set_decl_namespace or check_classfn set DECL_IMPLICIT_INSTANTIATION to
       indicate that we need a template match, so ask
       check_explicit_specialization to find one.  */
    decl = (check_explicit_specialization
	    (orig_declarator, decl, ctype_depth,
	     2 * funcdef_flag + 4));

  add_friend (current_class_type,
	      (!ctype && friend_depth) ? DECL_TI_TEMPLATE (decl) : decl,
	      /*complain=*/true);

  return decl;
}

#include "gt-cp-friend.h"
