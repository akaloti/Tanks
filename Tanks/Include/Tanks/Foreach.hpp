/*
  Authors of original version: Artur Moreira, Henrik Vogelius Hansson, and
    Jan Haller
*/

#ifndef TANKS_FOREACH_HPP
#define TANKS_FOREACH_HPP

// Preprocessor trick to allow nested loops
#define BOOK_PP_CAT_IMPL(a, b) a ## b
#define BOOK_PP_CAT(a, b) BOOK_PP_CAT_IMPL(a, b)
#define BOOK_ID(identifier) BOOK_PP_CAT(auroraDetail_, identifier)
#define BOOK_LINE_ID(identifier) BOOK_PP_CAT(BOOK_ID(identifier), __LINE__)


// Macro to emulate C++11 range-based for loop
// Instead of for (decl : range) you write FOREACH(decl, range) as in the following example
//
// std::vector<int> v = ...;
// FOREACH(int& i, v)
// {
//     i += 2;
// }
#define FOREACH(declaration, container)																											\
	if (bool BOOK_LINE_ID(broken) = false) {} else																								\
	for (auto BOOK_LINE_ID(itr) = (container).begin(); BOOK_LINE_ID(itr) != (container).end() && !BOOK_LINE_ID(broken); ++BOOK_LINE_ID(itr))	\
	if (bool BOOK_LINE_ID(passed) = false) {} else																								\
	if (BOOK_LINE_ID(broken) = true, false) {} else																								\
	for (declaration = *BOOK_LINE_ID(itr); !BOOK_LINE_ID(passed); BOOK_LINE_ID(passed) = true, BOOK_LINE_ID(broken) = false)

#endif // TANKS_FOREACH_HPP
