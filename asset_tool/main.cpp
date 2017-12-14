#include "asset_processor.h"

//#include <vld.h>

int main()
{
	pug::assets::InitAssetProcessor();
	pug::assets::RunAssetProcessor();
	pug::assets::DestroyAssetProcessor();

	return 0;
}
