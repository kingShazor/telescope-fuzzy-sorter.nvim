#include "simple_fuzzy_sorter.h"

#include <algorithm>
#include <array>
#include <cctype>
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
    if ( const auto pos = text.find( pattern ); pos != std::string::npos )
    {
      const uint patternSize = static_cast< uint >( pattern.size() );
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
    const size_t maxStartPos = text.size() - pattern.size() + 1;
    // static vectors are faster
    static vector< uint > positions;
    static vector< uint > resultPositions;
    positions.clear();
    resultPositions.clear();

    const uint maxScore = static_cast< uint >( pattern.size() * MATCH_CHAR );
    uint startSearchPos = 0;
    uint gap = 0;
    uint penalty = 0;
    for ( uint i = 0; i < maxStartPos; ++i )
    {
      penalty = 0;
      startSearchPos = i;
      uint maxVarStartPos = maxStartPos - 1;
      for ( const char patternChar : pattern )
      {
        uint pos = startSearchPos;
        ++maxVarStartPos;
        gap = 0;
        // find fuzzy position
        for ( ; pos < maxVarStartPos; ++pos )
        {
          // ignore blocked ranges
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
          if ( !positions.empty() )
          {
            ++gap;
            if ( gap > MAX_GAP )
            {
              if ( penalty == 0 )
                i = positions.back(); // Not 100% correct but very fast without penalty check
              else if ( positions.size() > 1 )
                i = positions[ 1 ]; // second position should be save s: "ABX" in "AAAB" will all greedy apply to B
              break;
            }
          }
        }
        if ( pos >= maxVarStartPos || gap > MAX_GAP )
          break;

        if ( positions.empty() )
          i = pos;
        else if ( gap > 0 )
          penalty += ( gap * static_cast< uint >( GAP_PENALTY ) );
        positions.push_back( pos );
        startSearchPos = pos + 1;
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
      blockedRanges->push_back( pair( resultPositions.front(), resultPositions.back() ) );

    if ( getPositions )
      return resultPositions;
    return score;
  }

  /*
   * This Function will be called within two steps: calcing score (first step) calcing positions to highlight characters
   * (seocnd step). Telescope uses discard mode, so MISMATCHs in step one will be discarded. So when positions are
   * calculated, we know that the pattern already matches.
   * Steps:
   *   -plit pattern into tokens. tokens with one sign or with upper case char well be searched strictly.
   *   -calc strict or fuzzy scors
   *   -put togehter multi token results
   * \param getPositions true: get positions instead of a rating
   */
  result_t get_score( const string_view &text, const char *pattern, const bool getPositions )
  {
    if ( pattern == nullptr || pattern[ 0 ] == '\0' ) // empty pattern must return match, because of discard
      return getPositions ? result_t{ vector< uint >() } : result_t{ FULL_MATCH };
    if ( pattern[ 1 ] == '\0' ) // this will be applied on all file-names, so this must be very fast
    {
      string_view p = pattern;
      if ( std::islower( p.back() ) )
      {
        const auto res = get_strict_score( text,
                                           string{
                                             static_cast< char >( std::toupper( static_cast< int >( p.back() ) ) ) },
                                           getPositions );
        if ( getPositions || std::get< int >( res ) != MISMATCH )
          return res;
      }

      return get_strict_score( text, pattern, getPositions );
    }

    const char sep = ' ';

    struct patternHelper_c
    {
      string_view pattern;
      // uint utf8size;
      bool strict;
    };

    // a small cache for the last pattern - so we don't need to create every check patternHelper
    static pair< string, vector< patternHelper_c > > cachePattern;
    vector< patternHelper_c > &patternHelpers = cachePattern.second;
    if ( cachePattern.first.compare( pattern ) != 0 )
    {
      cachePattern.first = pattern;
      const string_view patternString = cachePattern.first;
      patternHelpers.clear();
      bool strict = false;
      for ( uint i = 0; i < patternString.size(); ++i )
      {
        uint y = i;
        for ( ; y < patternString.size(); ++y )
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
          patternHelpers.push_back(
            patternHelper_c{ .pattern = patternString.substr( i, newPatternSize ), .strict = strict } );
          strict = false;
          i = y;
        }
      }
    }

    if ( cachePattern.first.size() > text.size() )
      return getPositions ? result_t{ vector< uint >() } : result_t{ MISMATCH };

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

namespace fuzzy_score_n
{
  // ma score is the best :)
  int fzs_get_score( const char *text, const char *pattern )
  {
    return std::get< int >( get_score( text, pattern, false ) );
  }
} // namespace fuzzy_score_n

// -------- C-Interface ----------

double fzs_get_score( const char *text, const char *pattern )
{
  const int score = fuzzy_score_n::fzs_get_score( text, pattern );
  if ( score == MISMATCH )
    return -1.0;

  return static_cast< double >( 1 ) / static_cast< double >( score );
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
