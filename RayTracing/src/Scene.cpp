#include "Scene.h"

glm::vec3 const& Triangle::VertexPositions::operator[](int i) const
{
	assert(i >= 0 && i < 3);
	switch (i)
	{
	case 0:
		return a;
	case 1:
		return b;
	case 2:
		return c;
	}
}
