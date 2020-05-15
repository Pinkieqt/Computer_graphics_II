#ifndef COLOR_H_
#define COLOR_H_

#include <assert.h>
#include <array>

/*! \class Color
\brief A simple templated representation of color.

RGB, RGBA, BGR, BGRA
uchar, float
 ||     ||
sRGB, linear

EXR float, RGB(A), linear
PNG uchar, RGB(A), sRGB

\author Tomáš Fabián
\version 1.0
\date 2020
*/

/*enum ColorSpaces
{
	kLinearRGB = 0,
	kSRGB,
};*/

#define FAST_INTERP
constexpr auto GAMMA = 2.4f;

template <int N, class T>
class Color
{
public:
	Color()
	{
		data.fill( T( 0 ) );
	}

	Color( const std::array<T, N> & data )
	{
		this->data = data;
	}
		
	Color<N, float> & operator+=( const Color<N, float> & rhs )
	{
		for ( int i = 0; i < N; ++i )
		{
			data[i] += rhs.data[i];
		}

		return *this;
	}
	
	friend Color<N, T> operator-( Color<N, T> lhs, const Color<N, T> & rhs )
	{
		// passing lhs by value helps optimize chained a + b + c
		// otherwise, both parameters may be const references
		lhs -= rhs;

		return lhs;
	}

	Color<N, float> & operator-=( const Color<N, float> & rhs )
	{
		for ( int i = 0; i < N; ++i )
		{
			data[i] -= rhs.data[i];
		}

		return *this;
	}

	friend Color<N, T> operator+( Color<N, T> lhs, const Color<N, T> & rhs )
	{
		// passing lhs by value helps optimize chained a + b + c
		// otherwise, both parameters may be const references
		lhs += rhs;

		return lhs;
	}

	Color<N, float> & operator*=( const T rhs )
	{
		for ( int i = 0; i < N; ++i )
		{
			data[i] *= rhs;
		}

		return *this;
	}	

	friend Color<N, T> operator*( Color<N, T> lhs, const T rhs )
	{
		lhs *= rhs;

		return lhs;
	}

	Color<N, float> & operator*=( const Color<N, float> rhs )
	{
		for ( int i = 0; i < N; ++i )
		{
			data[i] *= rhs.data[i];
		}

		return *this;
	}

	friend Color<N, T> operator*( Color<N, T> lhs, const Color<N, float> rhs )
	{
		lhs *= rhs;

		return lhs;
	}

	// explicit type casting operator uchar -> float, i.e. sRGB -> linear
	//template<class T2>
	explicit operator Color<N, float>() const
	{
		Color<N, float> lhs;

		for ( int i = 0; i < N; ++i )
		{
			lhs.data[i] = c_linear( float( data[i] ) * ( 1.0f / 255.0f ) );
		}

		return lhs;
	}

	// explicit type casting operator float -> uchar, i.e. linear -> sRGB
	explicit operator Color<N, unsigned char>() const
	{
		Color<N, unsigned char> lhs;

		for ( int i = 0; i < N; ++i )
		{
			lhs.data[i] = ( unsigned char )( std::max( T( 0 ), std::min( T( 255 ), c_srgb( data[i] ) * T( 255 ) ) ) );
		}

		return lhs;
	}

	bool is_zero() const
	{
		for ( int i = 0; i < N; ++i )
		{
			if ( data[i] != T( 0 ) ) return false;
		}

		return true;
	}

	bool is_valid( const T from = -T( 100000 ), const int to = T( 100000 ) ) const
	{
		for ( int i = 0; i < N; ++i )
		{
			if ( data[i] < from || data[i] > to ) return false;
		}

		return true;
	}

	void reverse()
	{
		std::reverse( std::begin( data ), std::end( data ) );
	}

	T max_value() const
	{
		T value = std::numeric_limits<T>::lowest();

		for ( int i = 0; i < N; ++i )
		{			
			if ( ( N != 4 || i != N - 1 ) && data[i] > value )
			{
				value = data[i];
			}
		}

		return value;
	}

	T min_value() const
	{
		T value = ( std::numeric_limits<T>::max )();

		for ( int i = 0; i < N; ++i )
		{
			if ( ( N != 4 || i != N - 1 ) && data[i] < value )
			{
				value = data[i];
			}
		}

		return value;
	}

	static float c_linear( const float c_srgb, const float gamma = GAMMA )
	{
		if ( c_srgb <= 0.0f ) return 0.0f;
		else if ( c_srgb >= 1.0f ) return 1.0f;

		assert( ( c_srgb >= 0.0f ) && ( c_srgb <= 1.0f ) );

		if ( c_srgb <= 0.04045f )
		{
			return c_srgb / 12.92f;
		}
		else
		{
			const float a = 0.055f;
			return powf( ( c_srgb + a ) / ( 1.0f + a ), gamma );
		}
	}

	static float c_srgb( const float c_linear, const float gamma = GAMMA )
	{
		if ( c_linear <= 0.0f ) return 0.0f;
		else if ( c_linear >= 1.0f ) return 1.0f;

		assert( ( c_linear >= 0.0f ) && ( c_linear <= 1.0f ) );

		if ( c_linear <= 0.0031308f )
		{
			return 12.92f * c_linear;
		}
		else
		{
			const float a = 0.055f;
			return ( 1.0f + a ) * powf( c_linear, 1.0f / gamma ) - a;
		}
	}

	template<int N>
	static Color<N, float> toLinear( Color<N, float> srgb )
	{
		Color<N, float> & linear = srgb;

		for ( int i = 0; i < srgb.channels; ++i )
		{
			linear.data[i] = Color<N, float>::c_linear( srgb.data[i] );
		}

		return linear;
	}

	template<int N>
	static Color<N, float> toSRGB( Color<N, float> linear )
	{
		Color<N, float> & srgb = linear;

		for ( int i = 0; i < srgb.channels; ++i )
		{
			srgb.data[i] = Color<N, float>::c_srgb( linear.data[i] );
		}

		return srgb;
	}

	std::array<T, N> data;
	static const int channels = N;
	//static const ColorSpaces colorSpace = ColorSpaces::kLinearRGB; // default color space
};

using Color3f = Color<3, float>;
using Color4f = Color<4, float>;
using Color3u = Color<3, unsigned char>;
using Color4u = Color<4, unsigned char>;

#endif
