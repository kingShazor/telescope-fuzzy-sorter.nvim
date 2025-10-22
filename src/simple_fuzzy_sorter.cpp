#include "simple_fuzzy_sorter.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <iostream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std;
using namespace fuzzy_score_n;

namespace
{
  enum
  {
    U_CHAR_SIZE = 256,
    BUFFER_SIZE = 100
  };

  // small extra bonus for matching sign after oder before the pattern
  vector< unsigned char > boundaryChars()
  {
    vector< unsigned char > boundaries( U_CHAR_SIZE, false );
    boundaries[ '-' ] = true;
    boundaries[ '_' ] = true;
    boundaries[ ' ' ] = true;
    boundaries[ '/' ] = true;
    boundaries[ '\\' ] = true;
    boundaries[ '(' ] = true;
    boundaries[ ')' ] = true;
    boundaries[ ']' ] = true;
    boundaries[ '[' ] = true;
    boundaries[ '.' ] = true;
    boundaries[ ':' ] = true;
    boundaries[ ';' ] = true;

    return boundaries;
  }

  // some nice debugging
  struct [[maybe_unused]] dbg
  {
    bool _addSep = false;

    ~dbg()
    {
      cout << endl;
    }

    template< class ARG >
    dbg &operator<<( const ARG &val )
    {
      if ( _addSep )
        cout << " ";
      else
        _addSep = true;

      cout << val;
      return *this;
    }
  };

  uint utf8_char_length( unsigned char c )
  {
    if ( ( c & 0x80 ) == 0x00 )
      return 1; // 0xxxxxxx -> 1-byte char
    if ( ( c & 0xE0 ) == 0xC0 )
      return 2; // 110xxxxx -> 2-byte char
    if ( ( c & 0xF0 ) == 0xE0 )
      return 3; // 1110xxxx -> 3-byte char
    if ( ( c & 0xF8 ) == 0xF0 )
      return 4; // 11110xxx -> 4-byte char
    return 1; // fallback
  }

  // end is after the last found sign.
  int scoreBoundary( const string_view &text, uint begin, uint end )
  {
    static const vector< unsigned char > boundary = boundaryChars();
    int score = 0;
    if ( begin == 0 || boundary[ static_cast< unsigned char >( text[ begin - 1 ] ) ] )
      score += 2;
    if ( end == text.size() || boundary[ static_cast< unsigned char >( text[ end ] ) ] )
      score += 2;

    return score;
  }

  using result_t = variant< int, vector< uint > >;

  /*
   * calcing a fast strict score (the pattern must match ascending).
   */
  result_t get_strict_score( const string_view &text, const string_view &pattern, const bool getPositions )
  {
    if ( const auto it = search( text.begin(),
                                 text.end(),
                                 boyer_moore_horspool_searcher( pattern.begin(), pattern.end() ) );
         it != text.end() )
    {
      const uint patternSize = static_cast< uint >( pattern.size() );
      const auto pos = static_cast< uint >( it - text.begin() );
      if ( getPositions )
      {
        vector< uint > positions;
        for ( uint x = pos; x < pos + patternSize; ++x )
          positions.push_back( x );
        return positions;
      }

      return FULL_MATCH - BOUNDARY_BOTH + scoreBoundary( text, pos, pos + patternSize );
    }

    if ( getPositions )
      return vector< uint >();
    return MISMATCH;
  }

  /*
   * fuzzy means: allowing gaps between found characters and looking also for uppercase chars
   *              we don't use UTF-8 here, because the overhead. It will only used for finding file_names
   *              so a 'ü' will have here two chars which need to match 'case sensitve'.
   *              Also meaning langugage chars count as a larger gap.
   *
   * \pattern        includes only lower case chars
   * \getPositions   true: return postions instead of score
   * \blockedRanges  if enabled only allow free spaces
   */
  result_t get_fuzzy_score( const string_view &text,
                            const string_view &pattern,
                            const bool getPositions,
                            vector< pair< uint, uint > > *blockedRanges = nullptr )
  {
    int score = MISMATCH;
    const size_t maxPos = text.size() - pattern.size();
    // static vectors are faster
    static vector< uint > positions;
    static vector< uint > resultPositions;
    positions.clear();
    resultPositions.clear();

    const uint maxScore = static_cast< uint >( pattern.size() * MATCH_CHAR );
    uint startPos = 0;
    for ( uint i = 0; i <= maxPos; ++i )
    {
      uint penalty = 0;
      startPos = i;
      for ( const char patternChar : pattern )
      {
        uint pos = startPos;
        // find fuzzy position
        for ( ; pos < text.size(); ++pos )
        {
          if ( blockedRanges && !blockedRanges->empty() )
          {
            bool matchRange = false;
            for ( auto &blockRange : *blockedRanges )
              if ( pos >= blockRange.first && pos <= blockRange.second )
              {
                pos = blockRange.second;
                matchRange = true;
                break;
              }
            if ( matchRange )
              continue;
          }
          char textChar = text[ pos ];
          if ( textChar > 0 && isupper( textChar ) )
            textChar = static_cast< char >( tolower( static_cast< unsigned char >( textChar ) ) );
          if ( patternChar == textChar )
            break;
        }
        if ( pos == text.size() )
          break;

        if ( positions.empty() )
          i = pos;
        else
        {
          if ( const uint gap = pos - startPos; gap > MAX_GAP )
            break;
          else if ( gap > 0 )
            penalty += ( gap * static_cast< uint >( GAP_PENALTY ) );
        }
        positions.push_back( pos );
        startPos = pos + 1;
      }

      // Impossible match, when first char can't be found
      if ( positions.empty() )
        break;
      if ( positions.size() == pattern.size() )
      {
        const int boundaryScore = scoreBoundary( text, positions.front(), positions.back() + 1 );
        if ( penalty == 0 && boundaryScore == BOUNDARY_BOTH )
        {
          std::swap( positions, resultPositions );
          score = FULL_MATCH;
          break;
        }

        const int newScore = static_cast< int >( pattern.size() * MATCH_CHAR - penalty );
        int normalizedScore = static_cast< int >(
          static_cast< float >( newScore ) / static_cast< float >( maxScore ) * 100.0f + 0.5f );
        normalizedScore += ( -BOUNDARY_BOTH + boundaryScore );
        if ( normalizedScore > score )
          std::swap( positions, resultPositions );
        score = max( normalizedScore, score );
      }
      positions.clear();
    }

    if ( score != MISMATCH && blockedRanges )
    {
      blockedRanges->push_back( pair( resultPositions.front(), resultPositions.back() ) );
      sort( blockedRanges->begin(), blockedRanges->end() );
    }

    if ( getPositions )
      return resultPositions;
    return score;
  }

  /*
   * split pattern into tokens. Small tokens or with upper case char are strict searched.
   * \param getPositions true: get positions instead of a rating
   */
  result_t get_score( const string_view &text, const string_view &pattern, const bool getPositions )
  {
    if ( pattern.empty() )
      return getPositions ? result_t{ vector< uint >() } : result_t{ FULL_MATCH };
    if ( pattern.size() > text.size() )
      return getPositions ? result_t{ vector< uint >() } : result_t{ MISMATCH };

    const char sep = ' ';

    struct patternHelper_c
    {
      string_view pattern;
      // uint utf8size;
      bool strict;
    };

    // creating a cache doesn't make sense: costs generating tokens ~= creating a cache
    vector< patternHelper_c > patternHelpers;
    bool strict = false;
    for ( uint i = 0; i < pattern.size(); ++i )
    {
      uint y = i;
      for ( ; y < pattern.size(); ++y )
      {
        const char c = pattern[ y ];
        uint byte_size = utf8_char_length( static_cast< unsigned char >( c ) );
        if ( byte_size == 1 ) // ASCII
        {
          const bool isSpace = c == sep;
          if ( isSpace )
            break;
          else if ( c > 0 && isupper( c ) )
            strict = true;
        }
        else
        {
          y += byte_size - 1; // y will be incremented to the next index to check via for-increment ++y
          strict = true;
        }
      }
      if ( uint newPatternSize = y - i; y > 0 )
      {
        patternHelpers.push_back( patternHelper_c{ .pattern = pattern.substr( i, newPatternSize ), .strict = strict } );
        strict = false;
        i = y;
      }
    }

    // optimization reason: reduce creation of empty vectors
    if ( patternHelpers.size() == 1 )
    {
      const auto &patternHelper = patternHelpers.back();
      return patternHelper.strict ? get_strict_score( text, patternHelper.pattern, getPositions )
                                  : get_fuzzy_score( text, patternHelper.pattern, getPositions );
    }

    // ugly but maybe a little bit faster
    result_t result = getPositions ? result_t{ std::in_place_type< vector< uint > > } : result_t{ MISMATCH };
    vector< pair< uint, uint > > range;
    for ( const auto &patternHelper : patternHelpers )
    {
      auto patternResult = patternHelper.strict ? get_strict_score( text, patternHelper.pattern, getPositions )
                                                : get_fuzzy_score( text, patternHelper.pattern, getPositions, &range );
      if ( getPositions )
      {
        auto &patternPositions = std::get< vector< uint > >( patternResult );
        if ( patternPositions.empty() )
          return patternPositions;
        auto &positions = std::get< vector< uint > >( result );
        if ( positions.empty() )
          std::swap( positions, patternPositions );
        else
          positions.insert( positions.end(), patternPositions.begin(), patternPositions.end() );
      }
      else
      {
        const int patternScore = std::get< int >( patternResult );

        if ( patternScore == MISMATCH )
          return MISMATCH;
        std::get< int >( result ) += patternScore;
      }
    }

    return result;
  }
} // namespace

// -------- C-Interface ----------

// ma score is the best :)
int fzs_get_score( const char *text, const char *pattern )
{
  return std::get< int >( get_score( text, pattern, false ) );
}

// positions will be displayed by the gui
fzs_position_t *fzs_get_positions( const char *text, const char *pattern )
{
  // save mem - nice trick :-)
  static array< uint, BUFFER_SIZE > array;
  static fzs_position_t result{ .data = array.data(), .size = 0 };
  const auto positions = std::get< vector< uint > >( get_score( text, pattern, true ) );

  const auto size = std::min( positions.size(), array.size() );
  for ( uint i = 0; i < size; ++i )
    result.data[ i ] = positions[ i ];

  result.size = static_cast< uint >( positions.size() );

  return &result;
}
