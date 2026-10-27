#ifndef C1_COMPILER_MESSAGE_IDS_H
#define C1_COMPILER_MESSAGE_IDS_H

/* Strings loaded by the finite compiler text catalog. */
#define C1_TEXT_UNNAMED_TOKEN                                    303
#define C1_TEXT_UNNAMED_LABEL                                    304
#define C1_TEXT_OLD_VARIADIC_COMMA                               305
#define C1_TEXT_BAD_NUMBER_SUFFIX                                306
#define C1_TEXT_UNKNOWN_WORD                                     307
#define C1_TEXT_UNKNOWN_LABEL                                    308
#define C1_TEXT_NOTE                                             312
#define C1_TEXT_WARNING                                          313
#define C1_TEXT_ERROR                                            314
#define C1_TEXT_FATAL_ERROR                                      315
#define C1_TEXT_FIRST_OPERAND                                    320
#define C1_TEXT_SECOND_OPERAND                                   321
#define C1_TEXT_THIRD_OPERAND                                    322
#define C1_TEXT_OPCODE                                           323
#define C1_TEXT_ANONYMOUS_UNION                                  333
#define C1_TEXT_MEMBER                                           334
#define C1_TEXT_ENUMERATOR                                       335
#define C1_TEXT_INACCESSIBLE                                     336
#define C1_TEXT_INPUT                                            337
#define C1_TEXT_NON_STATIC                                       338
#define C1_TEXT_COMPILER_INTERMEDIATE                            339
#define C1_TEXT_COMPILER_GENERATED                               340
#define C1_TEXT_SOURCE                                           341
#define C1_TEXT_INLINE_FUNCTION_DEFINITION                       342
#define C1_TEXT_INCLUDE                                          343
#define C1_TEXT_PRECOMPILED_HEADER                               344
#define C1_TEXT_OVERLOADED_FUNCTION_TYPE                         345
#define C1_TEXT_OPERATOR_NEW_TARGET                              346
#define C1_TEXT_SINGLE_INHERITANCE                               347
#define C1_TEXT_MULTIPLE_INHERITANCE                             348
#define C1_TEXT_VIRTUAL_INHERITANCE                              349
#define C1_TEXT_USING_PRECOMPILED_HEADER                         350
#define C1_TEXT_CREATING_PRECOMPILED_HEADER                      351
#define C1_TEXT_CREATING_NEW_PRECOMPILED_HEADER                  352
#define C1_TEXT_INCOMPATIBLE_OPTIONS                             353
#define C1_TEXT_PROGRAM_DATABASE                                 354
#define C1_TEXT_BAD_PATH                                         355
#define C1_TEXT_DEBUG                                            356
#define C1_TEXT_BROWSER                                          357
#define C1_TEXT_TYPE_LIBRARY                                     358
#define C1_TEXT_BRACE                                            360
#define C1_TEXT_PARENTHESIS                                      361
#define C1_TEXT_BRACKET                                          362
#define C1_TEXT_BLOCK_COMMENT                                    363
#define C1_TEXT_ANGLE_BRACKET                                    364
#define C1_TEXT_DEFAULT_CONSTRUCTOR                              370
#define C1_TEXT_COPY_CONSTRUCTOR                                 371
#define C1_TEXT_ASSIGNMENT_OPERATOR                              372
#define C1_TEXT_DESTRUCTOR                                       373
#define C1_TEXT_FUNCTION                                         374
#define C1_TEXT_EMPTY_DECLARATION                                375
#define C1_TEXT_STATIC_DATA_MEMBER                               376
#define C1_TEXT_DATA                                             377
#define C1_TEXT_FUNCTION_THROUGH_POINTER                         378
#define C1_TEXT_ARGUMENT                                         379
#define C1_TEXT_COULD_BE                                         380
#define C1_TEXT_OR                                               381
#define C1_TEXT_MEMORY_MODEL_OPTION                              390
#define C1_TEXT_DATABASE_NAME_OPTION                             391
#define C1_TEXT_NATIVE_CALLER_OPTION                             392
#define C1_TEXT_PASCAL_CALL_OPTION                               393
#define C1_TEXT_FASTCALL_OPTION                                  394
#define C1_TEXT_UNSIGNED_CHAR_OPTION                             395
#define C1_TEXT_MEMBER_POINTER_BEST_CASE_OPTION                  396
#define C1_TEXT_MEMBER_POINTER_GENERAL_OPTION                    397
#define C1_TEXT_MEMBER_POINTER_INHERITANCE_OPTION                398
#define C1_TEXT_VTORDISP_OPTION                                  399
#define C1_TEXT_DLL_RUNTIME_OPTION                               400
#define C1_TEXT_BITFIELD_LAYOUT_OPTION                           401
#define C1_TEXT_OPTIMIZATION_OPTIONS                             402
#define C1_TEXT_EXCEPTION_UNWINDING_OPTION                       403
#define C1_TEXT_BROWSER_OPTION                                   404
#define C1_TEXT_LIBRARY_PCT_OPTION                               405
#define C1_TEXT_PACKING_OPTION                                   406
#define C1_TEXT_RTTI_OPTION                                      407
#define C1_TEXT_INCLUDED_FILE_NOTE                               408
#define C1_TEXT_BOOL_OPTION                                      409
#define C1_TEXT_INCLUDE_TWO_TYPES_NOTE                           901
#define C1_TEXT_INCLUDE_TYPE_NOTE                                902
#define C1_TEXT_NEW_PCH_NOTE                                     903
#define C1_TEXT_OPTIONS_CHANGED_NOTE                             904
#define C1_TEXT_UPDATE_PCH_MEMORY_NOTE                           905
#define C1_TEXT_UPDATE_PCH_MEMORY_FAILED                         906
#define C1_TEXT_PCH_TIMESTAMP_FAILED                             907
#define C1_TEXT_REUSE_PCH_NOTE                                   908
#define C1_TEXT_MEMORY_RELEASE_NOTE                              910

/* Reached language warning, separate from the initialized text catalog. */
#define C1_TEXT_ILLEGAL_INITIALIZATION 2097
#define C1_TEXT_FASTCALL_STACK_CHECK 4124

#define C1_COMPILER_MESSAGE_COUNT 87
#include <recon.h>
#endif
