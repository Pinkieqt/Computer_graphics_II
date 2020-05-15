#include "pch.h"
#include "structs.h"
#include "mymath.h"

Coord2f operator+( const Coord2f & x, const Coord2f & y )
{
	return Coord2f{ x.u + y.u, x.v + y.v };
}

Coord2f operator-( const Coord2f & x, const Coord2f & y )
{
	return Coord2f{ x.u - y.u, x.v - y.v };
}


Vertex3f::operator Vector3() const
{
	return Vector3( x, y, z );
}

Normal3f Normal3f::operator*( const float a ) const
{
	return Normal3f{ x * a, y * a, z * a };
}
