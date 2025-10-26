#pragma once

using uint = unsigned int;

namespace fuzzy_score_n
{
  enum
  {
    MISMATCH = 0,
    FULL_MATCH = 100,
    BOUNDARY_WORD = 2,
    BOUNDARY_BOTH = BOUNDARY_WORD * 2,
    GAP_PENALTY = 5,
    MAX_GAP = 2,
    MAX_PENALTY = MAX_GAP * GAP_PENALTY,
    MATCH_CHAR = 10,
  };

  int fzs_get_score( const char *text, const char *pattern );
} // namespace fuzzy_score_n

extern "C"
{
  typedef struct
  {
    unsigned int *data;
    unsigned int size;
  } fzs_position_t;

  double fzs_get_score( const char *text, const char *pattern );
  fzs_position_t *fzs_get_positions( const char *text, const char *pattern );
}
