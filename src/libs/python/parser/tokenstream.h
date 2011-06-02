/*
 * Copyright (c) 2011 Instituto Nokia de Tecnologia
 *   Author: Paulo Alcantara <paulo.alcantara@openbossa.org>
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 as published by the
 * Free Software Foundation.
 */

#ifndef _TOKENSTREAM_H_
#define _TOKENSTREAM_H_

#include <QtCore/QtGlobal>

#include "locationtable.h"

namespace Python
{

class Driver;

class Token {
public:
    int kind;
    qint64 begin;
    qint64 end;

    Token()
        : kind(0), begin(0), end(0) {}
    virtual ~Token() {}

    bool is(int k) const { return k == kind; }
    bool isNot(int k) const { return k != kind; }
};

template<class T>
class TokenStreamBase
{
public:
  typedef T Token;
  TokenStreamBase()
    : mTokenBuffer(0),
      mTokenBufferSize(0),
      mIndex(0),
      mTokenCount(0),
      mLocationTable(0)
  {
    reset();
  }

  ~TokenStreamBase()
  {
    if (mTokenBuffer)
      ::free(mTokenBuffer);
    if (mLocationTable)
      delete mLocationTable;
  }

 inline void reset()
  {
    mIndex = 0;
    mTokenCount = 0;
  }

  inline void free()
  {
    mIndex = 0;
    mTokenCount = 0;
    ::free(mTokenBuffer);
  }

  inline qint64 size() const
  {
    return mTokenCount;
  }

  inline qint64 index() const
  {
    return mIndex;
  }

  inline qint64 tokenIndex() const
  {
    if( mIndex )
      return mIndex - 1;
    return mIndex;
  }

  inline void rewind(qint64 index)
  {
    mIndex = index;
  }

  /**
   * Returns the token at the specified position in the stream.
   */
  inline T const &token(qint64 index) const
  {
    return mTokenBuffer[index];
  }

  inline T &token(qint64 index)
  {
    return mTokenBuffer[index];
  }

  inline int nextToken()
  {
    return mTokenBuffer[mIndex++].kind;
  }

  inline T &next()
  {
    if (mTokenCount == mTokenBufferSize)
    {
      if (mTokenBufferSize == 0)
        mTokenBufferSize = 1024;

      mTokenBufferSize <<= 2;

      mTokenBuffer = reinterpret_cast<T*>
      (::realloc(mTokenBuffer, mTokenBufferSize * sizeof(T)));
    }

    return mTokenBuffer[mTokenCount++];
  }

  inline T &advance()
  {
    if (mIndex == mTokenCount)
    {
      if(mTokenCount++ == mTokenBufferSize)
      {
        if (mTokenBufferSize == 0)
          mTokenBufferSize = 1024;

        mTokenBufferSize <<= 2;

        mTokenBuffer = reinterpret_cast<T*>
        (::realloc(mTokenBuffer, mTokenBufferSize * sizeof(T)));
      }
    }

    return mTokenBuffer[mIndex++];
  }

  inline LocationTable *locationTable()
  {
    if (!mLocationTable)
      mLocationTable = new LocationTable();

    return mLocationTable;
  }

  inline void startPosition(qint64 index, qint64 *line, qint64 *column)
  {
    if (!mLocationTable)
      {
        *line = 0; *column = 0;
      }
    else
      mLocationTable->positionAt(token(index).begin, line, column);
  }

  inline void endPosition(qint64 index, qint64 *line, qint64 *column)
  {
    if (!mLocationTable)
      {
        *line = 0; *column = 0;
      }
    else
      mLocationTable->positionAt(token(index).end, line, column);
  }

private:
  T *mTokenBuffer;
  qint64 mTokenBufferSize;
  qint64 mIndex;
  qint64 mTokenCount;
  LocationTable *mLocationTable;

private:
  TokenStreamBase(TokenStreamBase const &other);
  void operator = (TokenStreamBase const &other);
};

class TokenStream : public TokenStreamBase<Token>
{
};

}

#endif /* _TOKENSTREAM_H_ */
