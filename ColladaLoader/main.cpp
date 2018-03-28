#include "SimpleCollada.h"

int main()
{
	SimpleCollada *importer = new SimpleCollada();

	importer->OpenColladaFile("assets/astroBoy_walk_Max.dae");

	importer->ImportColladaData();

}