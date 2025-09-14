#include "ray.h"

namespace Visor
{
	Ray::Ray(const Vector3<f32>& position, const Vector3<f32>& direction)
		: position(position)
		, direction(direction)
	{};

	b8 Ray::getIntersection(const AABB& AABB, Vector3<f32>& intersection)
	{
		f32 tmin = -1000000.0f;
		f32 tmax = 100000.0f;
		
		// Pour chaque axe
		for (int i = 0; i < 3; ++i)
		{
		    if (direction[i] != 0.0f)
		    {
		        f32 invD = 1.0f / direction[i];
		        f32 t0 = (AABB.minimum[i] - position[i]) * invD;
		        f32 t1 = (AABB.maximum[i] - position[i]) * invD;
		        if (t0 > t1) std::swap(t0, t1);		
		        tmin = std::max(tmin, t0);
		        tmax = std::min(tmax, t1);		
		        if (tmax < tmin) 
		            return false; // pas d’intersection
		    }
		    else
		    {
		        // Rayon parallèle à l’axe : il faut vérifier qu’il est dans la "tranche"
		        if (position[i] < AABB.minimum[i] || position[i] > AABB.maximum[i])
		            return false;
		    }
		}		
		if (tmax < 0.0f)
		    return false; // boîte est derrière le rayon		
		// Premier point d’intersection valide
		f32 tHit = (tmin >= 0.0f) ? tmin : tmax;
		intersection = position + direction * tHit;
		return true;
	}
}