#include "ITypeValueMap.h"
namespace gnsstk
{

	typeValueMap& TypeValueMapPtr::get_value() 
	{
		return *ptr;
	};

	const typeValueMap& TypeValueMapPtr::
		get_value() const
	{
		return *ptr;
	};
}