#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <vector>
#include <freeimage.h>
#include "color.h"
#include "pch.h"

FIBITMAP * BitmapFromFile( const char * file_name, int & width, int & height );
// Note that all float images in FreeImage are forced to have a range in <0, 1> after applying build-in conversions!!!
// see https://sourceforge.net/p/freeimage/bugs/259/
FIBITMAP * Custom_FreeImage_ConvertToRGBF( FIBITMAP * dib ); // this fix removes clamp from conversion of float images
FIBITMAP * Custom_FreeImage_ConvertToRGBAF( FIBITMAP * dib );  // this fix removes clamp from conversion of float images

/*! \class Texture
\brief A simple templated representation of texture.

Texture3f normal = Texture3f( "../../../data/denoise/normal_100spp.exr");
Texture3f output = Texture3f( width, height );
output.Save( "denoised.exr" );

\author Tomáš Fabián
\version 1.0
\date 2020
*/

template <class T, FREE_IMAGE_TYPE F>
class Texture
{
public:
	Texture( const int width, const int height )
	{
		assert( width > 0 && height > 0 );

		width_ = width;
		height_ = height;

		data_.resize( size_t( width ) * size_t( height ) );
	}

	Texture( const std::string & file_name )
	{		
		FIBITMAP * dib = BitmapFromFile( file_name.c_str(), width_, height_ );

		if ( dib )
		{			
			if ( true ) // always make sure that the loaded bitmap will fit the allocated data size
			{
				FIBITMAP * const dib_new = Convert( dib );
				assert( dib_new );
				FreeImage_Unload( dib );
				dib = dib_new;
			}			

			data_.resize( size_t( width_ ) * size_t( height_ ) );

			const int scan_width = FreeImage_GetPitch( dib ); // (bytes)
			const int bpp = FreeImage_GetBPP( dib ); // (bites)

			assert( bpp == sizeof( T ) * 8 );

			FreeImage_ConvertToRawBits( ( BYTE * )( data_.data() ), dib, scan_width, bpp,
				FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE );

			FreeImage_Unload( dib );
			dib = nullptr;

			double range[] = { ( std::numeric_limits<double>::max )( ), std::numeric_limits<double>::lowest() };

			for ( const auto & pixel : data_ )
			{
				range[0] = ( std::min )( range[0], double( pixel.min_value() ) );
				range[1] = ( std::max )( range[1], double( pixel.max_value() ) );				
			}			

			printf( "Texture '%s' (%d x %d px, %d bpp, <%0.3f, %0.3f>, %0.1f MB) loaded.\n",
				file_name.c_str(), width_, height_, bpp, range[0], range[1],
				scan_width * height_ / ( 1024.0f * 1024.0f ) );
		}
		else
		{
			printf( "Texture '%s' not loaded.\n", file_name.c_str() );
		}
	}

	T pixel( const int x, const int y ) const
	{
		assert( x >= 0 && x < width_ && y >= 0 && y < height_ );

		return data_[size_t( x ) + size_t( y ) * size_t( width_ )];
	}

	T texel( const float u, const float v ) const
	{
		T value;

#ifdef FAST_INTERP
		value = pixel( int( u * width_ ), int( v * height_ ) ); // nearest neighbour
#else
		// TODO bilinear interpolation	
#endif

		return value;
	}

	int width() const
	{
		return width_;
	}

	int height() const
	{
		return height_;
	}

	T * data()
	{
		return data_.data();
	}

	FIBITMAP * Convert( FIBITMAP * dib )
	{
		throw "Convert method is defined only for particular Texture types";

		return nullptr;
	}

	/*static Texture Load( const std::string & file_name )
	{
		throw "Load method is defined only for particular Texture types";
	}*/

	void Save( const std::string & file_name ) const
	{
		FIBITMAP * bitmap = FreeImage_AllocateT( F, width_, height_, sizeof( T ) * 8 ); // FIT_BITMAP, FIT_BITMAP, FIT_RGBF, FIT_RGBAF
		BYTE * data = ( BYTE * )( FreeImage_GetBits( bitmap ) );
		const int scan_width = FreeImage_GetPitch( bitmap );
		memcpy( data, data_.data(), scan_width * height_ );
		FreeImage_FlipVertical( bitmap );
		FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename( file_name.c_str() );
		if ( FreeImage_Save( fif, bitmap, file_name.c_str() ) )
		{
			printf( "Texture has been saved successfully in '%s'.\n", file_name.c_str() );
		}
		else
		{
			printf( "Texture failed to save in '%s'.\n", file_name.c_str() );
		}
		FreeImage_Unload( bitmap );
		bitmap = nullptr;
		data = nullptr;
	}

private:
	std::vector<T> data_;

	int width_{ 0 };
	int height_{ 0 };
};

using Texture3f = Texture<Color3f, FIT_RGBF>;
using Texture4f = Texture<Color4f, FIT_RGBAF>;
using Texture3u = Texture<Color3u, FIT_BITMAP>;
using Texture4u = Texture<Color4u, FIT_BITMAP>;

template<>
FIBITMAP * Texture3u::Convert( FIBITMAP * dib )
{
	return FreeImage_ConvertTo24Bits( dib );
}

template<>
FIBITMAP * Texture4u::Convert( FIBITMAP * dib )
{
	return FreeImage_ConvertTo32Bits( dib );
}

template<>
FIBITMAP * Texture3f::Convert( FIBITMAP * dib )
{
	return Custom_FreeImage_ConvertToRGBF( dib );
}

template<>
FIBITMAP * Texture4f::Convert( FIBITMAP * dib )
{
	return Custom_FreeImage_ConvertToRGBAF( dib );
}

#endif
