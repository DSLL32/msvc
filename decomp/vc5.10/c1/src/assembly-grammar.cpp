/* Target-derived inline assembler grammar: logical relations, not yacc compression.
 * Regenerate with scripts/assembly-grammar.py. State, production and
 * unknown nonterminal identities remain numbered until recovered. */
#include "assembly-grammar.h"
#include <recon.h>

const AssemblyParserTransition assembly_shifts[] = {
    /* State 0. */
    { ASSEMBLY_CLASS_32, 6 },
    { ASSEMBLY_ERROR, 4 },
    /* State 1. */
    { ASSEMBLY_CLASS_32, 6 },
    { ASSEMBLY_ERROR, 4 },
    /* State 3. */
    { ASSEMBLY_CLASS_34, 8 },
    /* State 4. */
    { ASSEMBLY_CLASS_34, 9 },
    /* State 5. */
    { ASSEMBLY_CLASS_44, 11 },
    /* State 10. */
    { ASSEMBLY_CLASS_35, 15 },
    { ASSEMBLY_CLASS_47, 17 },
    /* State 11. */
    { ASSEMBLY_CLASS_26, 18 },
    /* State 13. */
    { ASSEMBLY_CLASS_49, 19 },
    /* State 14. */
    { ASSEMBLY_CLASS_48, 20 },
    /* State 16. */
    { ASSEMBLY_CLASS_47, 21 },
    /* State 19. */
    { ASSEMBLY_CLASS_45, 23 },
    /* State 24. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 26. */
    { ASSEMBLY_CLASS_27, 51 },
    /* State 28. */
    { ASSEMBLY_NO_TOKEN, 22 },
    { ASSEMBLY_CLASS_1, 57 },
    { ASSEMBLY_CLASS_2, 58 },
    { ASSEMBLY_CLASS_3, 59 },
    { ASSEMBLY_CLASS_4, 60 },
    { ASSEMBLY_CLASS_5, 61 },
    { ASSEMBLY_CLASS_6, 62 },
    { ASSEMBLY_CLASS_7, 56 },
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_10, 54 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_13, 55 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 29. */
    { ASSEMBLY_CLASS_28, 72 },
    { ASSEMBLY_CLASS_43, 71 },
    { ASSEMBLY_CLASS_44, 70 },
    /* State 30. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 31. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 32. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 33. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 34. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 35. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 36. */
    { ASSEMBLY_CLASS_26, 79 },
    { ASSEMBLY_CLASS_30, 80 },
    /* State 37. */
    { ASSEMBLY_CLASS_40, 83 },
    { ASSEMBLY_CLASS_42, 82 },
    { ASSEMBLY_CLASS_43, 81 },
    { ASSEMBLY_CLASS_44, 84 },
    /* State 38. */
    { ASSEMBLY_CLASS_44, 85 },
    /* State 39. */
    { ASSEMBLY_CLASS_44, 86 },
    /* State 43. */
    { ASSEMBLY_CLASS_44, 87 },
    /* State 44. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 45. */
    { ASSEMBLY_CLASS_18, 89 },
    /* State 46. */
    { ASSEMBLY_CLASS_18, 90 },
    /* State 47. */
    { ASSEMBLY_CLASS_18, 91 },
    { ASSEMBLY_CLASS_44, 92 },
    /* State 48. */
    { ASSEMBLY_CLASS_18, 93 },
    { ASSEMBLY_CLASS_44, 94 },
    /* State 49. */
    { ASSEMBLY_CLASS_29, 95 },
    /* State 52. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 53. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 54. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 55. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 56. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 57. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 58. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 59. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 60. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 61. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 62. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 63. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 64. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 65. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 66. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 67. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 68. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 70. */
    { ASSEMBLY_CLASS_35, 116 },
    /* State 71. */
    { ASSEMBLY_CLASS_35, 117 },
    /* State 73. */
    { ASSEMBLY_CLASS_1, 57 },
    { ASSEMBLY_CLASS_2, 58 },
    { ASSEMBLY_CLASS_3, 59 },
    { ASSEMBLY_CLASS_4, 60 },
    { ASSEMBLY_CLASS_5, 61 },
    { ASSEMBLY_CLASS_6, 62 },
    { ASSEMBLY_CLASS_7, 56 },
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_10, 54 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_13, 55 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_31, 119 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 74. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 75. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 76. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 77. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 78. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 79. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 80. */
    { ASSEMBLY_CLASS_45, 121 },
    /* State 88. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 89. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 90. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 91. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 93. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 96. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 97. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 98. */
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 99. */
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 100. */
    { ASSEMBLY_CLASS_1, 57 },
    { ASSEMBLY_CLASS_2, 58 },
    { ASSEMBLY_CLASS_3, 59 },
    { ASSEMBLY_CLASS_4, 60 },
    { ASSEMBLY_CLASS_5, 61 },
    { ASSEMBLY_CLASS_6, 62 },
    { ASSEMBLY_CLASS_7, 56 },
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 101. */
    { ASSEMBLY_CLASS_1, 57 },
    { ASSEMBLY_CLASS_2, 58 },
    { ASSEMBLY_CLASS_3, 59 },
    { ASSEMBLY_CLASS_4, 60 },
    { ASSEMBLY_CLASS_5, 61 },
    { ASSEMBLY_CLASS_6, 62 },
    { ASSEMBLY_CLASS_7, 56 },
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 102. */
    { ASSEMBLY_CLASS_1, 57 },
    { ASSEMBLY_CLASS_2, 58 },
    { ASSEMBLY_CLASS_3, 59 },
    { ASSEMBLY_CLASS_4, 60 },
    { ASSEMBLY_CLASS_5, 61 },
    { ASSEMBLY_CLASS_6, 62 },
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 103. */
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 104. */
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 105. */
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 106. */
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 107. */
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 108. */
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 109. */
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 110. */
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 111. */
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 112. */
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 113. */
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 114. */
    { ASSEMBLY_CLASS_35, 69 },
    /* State 115. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 118. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 120. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 121. */
    { ASSEMBLY_CLASS_31, 132 },
    /* State 122. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 123. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 124. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 125. */
    { ASSEMBLY_CLASS_28, 72 },
    /* State 126. */
    { ASSEMBLY_CLASS_1, 57 },
    { ASSEMBLY_CLASS_2, 58 },
    { ASSEMBLY_CLASS_3, 59 },
    { ASSEMBLY_CLASS_4, 60 },
    { ASSEMBLY_CLASS_5, 61 },
    { ASSEMBLY_CLASS_6, 62 },
    { ASSEMBLY_CLASS_7, 56 },
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_10, 54 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_13, 55 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_29, 133 },
    { ASSEMBLY_CLASS_35, 69 },
    /* State 129. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 130. */
    { ASSEMBLY_CLASS_9, 31 },
    { ASSEMBLY_CLASS_14, 34 },
    { ASSEMBLY_CLASS_15, 39 },
    { ASSEMBLY_CLASS_16, 35 },
    { ASSEMBLY_CLASS_17, 44 },
    { ASSEMBLY_CLASS_19, 43 },
    { ASSEMBLY_CLASS_20, 38 },
    { ASSEMBLY_CLASS_21, 37 },
    { ASSEMBLY_CLASS_22, 32 },
    { ASSEMBLY_CLASS_23, 33 },
    { ASSEMBLY_CLASS_28, 50 },
    { ASSEMBLY_CLASS_30, 30 },
    { ASSEMBLY_CLASS_39, 36 },
    { ASSEMBLY_CLASS_40, 45 },
    { ASSEMBLY_CLASS_41, 48 },
    { ASSEMBLY_CLASS_42, 47 },
    { ASSEMBLY_CLASS_43, 46 },
    { ASSEMBLY_CLASS_44, 42 },
    { ASSEMBLY_CLASS_45, 40 },
    { ASSEMBLY_CLASS_46, 41 },
    /* State 131. */
    { ASSEMBLY_NO_TOKEN, 22 },
    { ASSEMBLY_CLASS_1, 57 },
    { ASSEMBLY_CLASS_2, 58 },
    { ASSEMBLY_CLASS_3, 59 },
    { ASSEMBLY_CLASS_4, 60 },
    { ASSEMBLY_CLASS_5, 61 },
    { ASSEMBLY_CLASS_6, 62 },
    { ASSEMBLY_CLASS_7, 56 },
    { ASSEMBLY_CLASS_8, 65 },
    { ASSEMBLY_CLASS_10, 54 },
    { ASSEMBLY_CLASS_11, 63 },
    { ASSEMBLY_CLASS_12, 64 },
    { ASSEMBLY_CLASS_13, 55 },
    { ASSEMBLY_CLASS_18, 68 },
    { ASSEMBLY_CLASS_22, 66 },
    { ASSEMBLY_CLASS_23, 67 },
    { ASSEMBLY_CLASS_24, 52 },
    { ASSEMBLY_CLASS_25, 53 },
    { ASSEMBLY_CLASS_35, 69 },
};

const AssemblyParserTransition assembly_exceptions[] = {
    { ASSEMBLY_END, -1 },
    { ASSEMBLY_CLASS_34, 6 },
    { ASSEMBLY_CLASS_35, 6 },
    { ASSEMBLY_CLASS_44, 6 },
    { ASSEMBLY_CLASS_47, 6 },
    { ASSEMBLY_CLASS_48, 6 },
    { ASSEMBLY_CLASS_49, 6 },
    { ASSEMBLY_CLASS_48, 18 },
    { ASSEMBLY_CLASS_49, 14 },
};

const AssemblyParserState assembly_states[ASSEMBLY_STATE_COUNT] = {
    { 6, 1, 0, 2, 0, 0 }, /* state 0 */
    { 0, 1, 2, 2, 0, 7 }, /* state 1 */
    { 1, 0, 4, 0, 7, 0 }, /* state 2 */
    { 0, 1, 4, 1, 7, 0 }, /* state 3 */
    { 0, 1, 5, 1, 7, 0 }, /* state 4 */
    { 8, 1, 6, 1, 7, 0 }, /* state 5 */
    { 7, 0, 7, 0, 7, 0 }, /* state 6 */
    { 2, 0, 7, 0, 7, 0 }, /* state 7 */
    { 3, 0, 7, 0, 7, 0 }, /* state 8 */
    { 4, 0, 7, 0, 7, 0 }, /* state 9 */
    { 10, 1, 7, 2, 7, 2 }, /* state 10 */
    { 0, 1, 9, 1, 9, 0 }, /* state 11 */
    { 5, 0, 10, 0, 9, 0 }, /* state 12 */
    { 0, 1, 10, 1, 9, 0 }, /* state 13 */
    { 0, 1, 11, 1, 9, 0 }, /* state 14 */
    { 15, 0, 12, 0, 9, 0 }, /* state 15 */
    { 19, 1, 12, 1, 9, 0 }, /* state 16 */
    { 20, 0, 13, 0, 9, 0 }, /* state 17 */
    { 9, 0, 13, 0, 9, 0 }, /* state 18 */
    { 16, 1, 13, 1, 9, 0 }, /* state 19 */
    { 12, 0, 14, 0, 9, 0 }, /* state 20 */
    { 21, 0, 14, 0, 9, 0 }, /* state 21 */
    { 11, 0, 14, 0, 9, 0 }, /* state 22 */
    { 17, 0, 14, 0, 9, 0 }, /* state 23 */
    { 22, 1, 14, 20, 9, 0 }, /* state 24 */
    { 13, 0, 34, 0, 9, 0 }, /* state 25 */
    { 23, 1, 34, 1, 9, 0 }, /* state 26 */
    { 24, 0, 35, 0, 9, 0 }, /* state 27 */
    { 27, 1, 35, 19, 9, 0 }, /* state 28 */
    { 51, 1, 54, 3, 9, 0 }, /* state 29 */
    { 0, 1, 57, 20, 9, 0 }, /* state 30 */
    { 0, 1, 77, 20, 9, 0 }, /* state 31 */
    { 0, 1, 97, 20, 9, 0 }, /* state 32 */
    { 0, 1, 117, 20, 9, 0 }, /* state 33 */
    { 0, 1, 137, 20, 9, 0 }, /* state 34 */
    { 0, 1, 157, 20, 9, 0 }, /* state 35 */
    { 70, 1, 177, 2, 9, 0 }, /* state 36 */
    { 0, 1, 179, 4, 9, 0 }, /* state 37 */
    { 0, 1, 183, 1, 9, 0 }, /* state 38 */
    { 0, 1, 184, 1, 9, 0 }, /* state 39 */
    { 65, 0, 185, 0, 9, 0 }, /* state 40 */
    { 66, 0, 185, 0, 9, 0 }, /* state 41 */
    { 67, 0, 185, 0, 9, 0 }, /* state 42 */
    { 0, 1, 185, 1, 9, 0 }, /* state 43 */
    { 0, 1, 186, 20, 9, 0 }, /* state 44 */
    { 0, 1, 206, 1, 9, 0 }, /* state 45 */
    { 0, 1, 207, 1, 9, 0 }, /* state 46 */
    { 0, 1, 208, 2, 9, 0 }, /* state 47 */
    { 0, 1, 210, 2, 9, 0 }, /* state 48 */
    { 0, 1, 212, 1, 9, 0 }, /* state 49 */
    { 79, 0, 213, 0, 9, 0 }, /* state 50 */
    { 25, 0, 213, 0, 9, 0 }, /* state 51 */
    { 0, 1, 213, 20, 9, 0 }, /* state 52 */
    { 0, 1, 233, 20, 9, 0 }, /* state 53 */
    { 0, 1, 253, 20, 9, 0 }, /* state 54 */
    { 0, 1, 273, 20, 9, 0 }, /* state 55 */
    { 0, 1, 293, 20, 9, 0 }, /* state 56 */
    { 0, 1, 313, 20, 9, 0 }, /* state 57 */
    { 0, 1, 333, 20, 9, 0 }, /* state 58 */
    { 0, 1, 353, 20, 9, 0 }, /* state 59 */
    { 0, 1, 373, 20, 9, 0 }, /* state 60 */
    { 0, 1, 393, 20, 9, 0 }, /* state 61 */
    { 0, 1, 413, 20, 9, 0 }, /* state 62 */
    { 0, 1, 433, 20, 9, 0 }, /* state 63 */
    { 0, 1, 453, 20, 9, 0 }, /* state 64 */
    { 0, 1, 473, 20, 9, 0 }, /* state 65 */
    { 0, 1, 493, 20, 9, 0 }, /* state 66 */
    { 0, 1, 513, 20, 9, 0 }, /* state 67 */
    { 0, 1, 533, 20, 9, 0 }, /* state 68 */
    { 49, 0, 553, 0, 9, 0 }, /* state 69 */
    { 0, 1, 553, 1, 9, 0 }, /* state 70 */
    { 0, 1, 554, 1, 9, 0 }, /* state 71 */
    { 81, 0, 555, 0, 9, 0 }, /* state 72 */
    { 0, 1, 555, 19, 9, 0 }, /* state 73 */
    { 53, 1, 574, 1, 9, 0 }, /* state 74 */
    { 54, 1, 575, 1, 9, 0 }, /* state 75 */
    { 55, 1, 576, 1, 9, 0 }, /* state 76 */
    { 56, 1, 577, 1, 9, 0 }, /* state 77 */
    { 57, 1, 578, 1, 9, 0 }, /* state 78 */
    { 0, 1, 579, 20, 9, 0 }, /* state 79 */
    { 0, 1, 599, 1, 9, 0 }, /* state 80 */
    { 59, 0, 600, 0, 9, 0 }, /* state 81 */
    { 60, 0, 600, 0, 9, 0 }, /* state 82 */
    { 61, 0, 600, 0, 9, 0 }, /* state 83 */
    { 62, 0, 600, 0, 9, 0 }, /* state 84 */
    { 63, 0, 600, 0, 9, 0 }, /* state 85 */
    { 64, 0, 600, 0, 9, 0 }, /* state 86 */
    { 68, 0, 600, 0, 9, 0 }, /* state 87 */
    { 69, 1, 600, 1, 9, 0 }, /* state 88 */
    { 0, 1, 601, 20, 9, 0 }, /* state 89 */
    { 0, 1, 621, 20, 9, 0 }, /* state 90 */
    { 0, 1, 641, 20, 9, 0 }, /* state 91 */
    { 76, 0, 661, 0, 9, 0 }, /* state 92 */
    { 0, 1, 661, 20, 9, 0 }, /* state 93 */
    { 77, 0, 681, 0, 9, 0 }, /* state 94 */
    { 78, 0, 681, 0, 9, 0 }, /* state 95 */
    { 0, 1, 681, 20, 9, 0 }, /* state 96 */
    { 0, 1, 701, 20, 9, 0 }, /* state 97 */
    { 28, 1, 721, 2, 9, 0 }, /* state 98 */
    { 29, 1, 723, 2, 9, 0 }, /* state 99 */
    { 30, 1, 725, 16, 9, 0 }, /* state 100 */
    { 31, 1, 741, 16, 9, 0 }, /* state 101 */
    { 32, 1, 757, 15, 9, 0 }, /* state 102 */
    { 33, 1, 772, 9, 9, 0 }, /* state 103 */
    { 34, 1, 781, 9, 9, 0 }, /* state 104 */
    { 35, 1, 790, 9, 9, 0 }, /* state 105 */
    { 36, 1, 799, 9, 9, 0 }, /* state 106 */
    { 37, 1, 808, 9, 9, 0 }, /* state 107 */
    { 38, 1, 817, 9, 9, 0 }, /* state 108 */
    { 39, 1, 826, 2, 9, 0 }, /* state 109 */
    { 40, 1, 828, 2, 9, 0 }, /* state 110 */
    { 41, 1, 830, 2, 9, 0 }, /* state 111 */
    { 42, 1, 832, 7, 9, 0 }, /* state 112 */
    { 43, 1, 839, 7, 9, 0 }, /* state 113 */
    { 44, 1, 846, 1, 9, 0 }, /* state 114 */
    { 0, 1, 847, 20, 9, 0 }, /* state 115 */
    { 45, 0, 867, 0, 9, 0 }, /* state 116 */
    { 47, 0, 867, 0, 9, 0 }, /* state 117 */
    { 0, 1, 867, 20, 9, 0 }, /* state 118 */
    { 52, 0, 887, 0, 9, 0 }, /* state 119 */
    { 58, 1, 887, 1, 9, 0 }, /* state 120 */
    { 0, 1, 888, 1, 9, 0 }, /* state 121 */
    { 72, 1, 889, 1, 9, 0 }, /* state 122 */
    { 73, 1, 890, 1, 9, 0 }, /* state 123 */
    { 74, 1, 891, 1, 9, 0 }, /* state 124 */
    { 75, 1, 892, 1, 9, 0 }, /* state 125 */
    { 0, 1, 893, 19, 9, 0 }, /* state 126 */
    { 26, 0, 912, 0, 9, 0 }, /* state 127 */
    { 50, 0, 912, 0, 9, 0 }, /* state 128 */
    { 0, 1, 912, 20, 9, 0 }, /* state 129 */
    { 0, 1, 932, 20, 9, 0 }, /* state 130 */
    { 82, 1, 952, 19, 9, 0 }, /* state 131 */
    { 71, 0, 971, 0, 9, 0 }, /* state 132 */
    { 80, 0, 971, 0, 9, 0 }, /* state 133 */
    { 46, 0, 971, 0, 9, 0 }, /* state 134 */
    { 48, 0, 971, 0, 9, 0 }, /* state 135 */
};

const AssemblyParserGoto assembly_gotos[ASSEMBLY_NONTERMINAL_COUNT] = {
    { 6, 0, 0 }, /* nonterminal 0 */
    { 22, 0, 0 }, /* nonterminal 1 */
    { 28, 0, 23 }, /* nonterminal 2 */
    { 29, 23, 11 }, /* nonterminal 3 */
    { 49, 34, 0 }, /* nonterminal 4 */
    { 1, 34, 0 }, /* nonterminal 5 */
    { 2, 34, 1 }, /* nonterminal 6 */
    { 3, 35, 0 }, /* nonterminal 7 */
    { 5, 35, 0 }, /* nonterminal 8 */
    { 10, 35, 0 }, /* nonterminal 9 */
    { 12, 35, 0 }, /* nonterminal 10 */
    { 13, 35, 0 }, /* nonterminal 11 */
    { 14, 35, 0 }, /* nonterminal 12 */
    { 24, 35, 0 }, /* nonterminal 13 */
    { 25, 35, 0 }, /* nonterminal 14 */
    { 16, 35, 0 }, /* nonterminal 15 */
    { 26, 35, 0 }, /* nonterminal 16 */
    { 27, 35, 1 }, /* nonterminal 17 */
    { 97, 36, 0 }, /* nonterminal 18 */
    { 129, 36, 0 }, /* nonterminal 19 */
    { 130, 36, 0 }, /* nonterminal 20 */
    { 115, 36, 0 }, /* nonterminal 21 */
    { 96, 36, 0 }, /* nonterminal 22 */
    { 118, 36, 0 }, /* nonterminal 23 */
};

const AssemblyParserTransition assembly_goto_transitions[] = {
    /* Nonterminal 2: source state, destination. */
    { 30, 73 },
    { 52, 98 },
    { 53, 99 },
    { 54, 100 },
    { 55, 101 },
    { 56, 102 },
    { 57, 103 },
    { 58, 104 },
    { 59, 105 },
    { 60, 106 },
    { 61, 107 },
    { 62, 108 },
    { 63, 109 },
    { 64, 110 },
    { 65, 111 },
    { 66, 112 },
    { 67, 113 },
    { 68, 114 },
    { 96, 126 },
    { 115, 128 },
    { 118, 131 },
    { 129, 134 },
    { 130, 135 },
    /* Nonterminal 3: source state, destination. */
    { 31, 74 },
    { 32, 75 },
    { 33, 76 },
    { 34, 77 },
    { 35, 78 },
    { 44, 88 },
    { 79, 120 },
    { 89, 122 },
    { 90, 123 },
    { 91, 124 },
    { 93, 125 },
    /* Nonterminal 6: source state, destination. */
    { 1, 7 },
    /* Nonterminal 17: source state, destination. */
    { 97, 127 },
};

const AssemblyParserProduction assembly_productions[ASSEMBLY_PRODUCTION_COUNT + 1] = {
    { 0, 0 }, /* production 0 */
    { 5, 1 }, /* production 1 */
    { 5, 2 }, /* production 2 */
    { 6, 2 }, /* production 3 */
    { 6, 2 }, /* production 4 */
    { 7, 3 }, /* production 5 */
    { 8, 0 }, /* production 6 */
    { 8, 1 }, /* production 7 */
    { 9, 0 }, /* production 8 */
    { 9, 2 }, /* production 9 */
    { 10, 0 }, /* production 10 */
    { 10, 3 }, /* production 11 */
    { 13, 0 }, /* production 12 */
    { 10, 4 }, /* production 13 */
    { 11, 0 }, /* production 14 */
    { 11, 1 }, /* production 15 */
    { 1, 0 }, /* production 16 */
    { 1, 1 }, /* production 17 */
    { 12, 0 }, /* production 18 */
    { 12, 1 }, /* production 19 */
    { 15, 1 }, /* production 20 */
    { 15, 2 }, /* production 21 */
    { 14, 0 }, /* production 22 */
    { 14, 1 }, /* production 23 */
    { 16, 1 }, /* production 24 */
    { 18, 0 }, /* production 25 */
    { 16, 4 }, /* production 26 */
    { 17, 1 }, /* production 27 */
    { 2, 3 }, /* production 28 */
    { 2, 3 }, /* production 29 */
    { 2, 3 }, /* production 30 */
    { 2, 3 }, /* production 31 */
    { 2, 3 }, /* production 32 */
    { 2, 3 }, /* production 33 */
    { 2, 3 }, /* production 34 */
    { 2, 3 }, /* production 35 */
    { 2, 3 }, /* production 36 */
    { 2, 3 }, /* production 37 */
    { 2, 3 }, /* production 38 */
    { 2, 3 }, /* production 39 */
    { 2, 3 }, /* production 40 */
    { 2, 3 }, /* production 41 */
    { 2, 3 }, /* production 42 */
    { 2, 3 }, /* production 43 */
    { 2, 3 }, /* production 44 */
    { 19, 0 }, /* production 45 */
    { 2, 5 }, /* production 46 */
    { 20, 0 }, /* production 47 */
    { 2, 5 }, /* production 48 */
    { 21, 0 }, /* production 49 */
    { 2, 4 }, /* production 50 */
    { 2, 1 }, /* production 51 */
    { 3, 3 }, /* production 52 */
    { 3, 2 }, /* production 53 */
    { 3, 2 }, /* production 54 */
    { 3, 2 }, /* production 55 */
    { 3, 2 }, /* production 56 */
    { 3, 2 }, /* production 57 */
    { 3, 3 }, /* production 58 */
    { 3, 2 }, /* production 59 */
    { 3, 2 }, /* production 60 */
    { 3, 2 }, /* production 61 */
    { 3, 2 }, /* production 62 */
    { 3, 2 }, /* production 63 */
    { 3, 2 }, /* production 64 */
    { 3, 1 }, /* production 65 */
    { 3, 1 }, /* production 66 */
    { 3, 1 }, /* production 67 */
    { 3, 2 }, /* production 68 */
    { 3, 2 }, /* production 69 */
    { 3, 1 }, /* production 70 */
    { 3, 4 }, /* production 71 */
    { 3, 3 }, /* production 72 */
    { 3, 3 }, /* production 73 */
    { 3, 3 }, /* production 74 */
    { 3, 3 }, /* production 75 */
    { 3, 2 }, /* production 76 */
    { 3, 2 }, /* production 77 */
    { 3, 2 }, /* production 78 */
    { 22, 0 }, /* production 79 */
    { 3, 4 }, /* production 80 */
    { 23, 0 }, /* production 81 */
    { 4, 4 }, /* production 82 */
};

const AssemblyParserRecovery assembly_recoveries[2] = {
    { 3, ASSEMBLY_CLASS_34, 8 },
    { 4, ASSEMBLY_CLASS_34, 9 },
};
