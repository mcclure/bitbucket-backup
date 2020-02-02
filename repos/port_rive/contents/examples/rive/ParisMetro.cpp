
#include "Metro.h"
#include "Game.h"

using namespace ld;

//--------------------------------------------------------------------
static void _register(std::map<std::wstring, Station*> &stations, Station *station)
{
	stations.insert(std::pair<std::wstring, Station*>(station->name, station));
}

//---------------------------------------------------------------------
static void _register(Station *station, Stretch &stretch)
{
	bool newLigne = true;

	// check if this is a new ligne
	for (auto &s : station->stretches)
	{
		if (s->ligne == stretch.ligne)
		{
			newLigne = false;
			break;
		}
	}

	// register
	station->stretches.push_back(&stretch);

	// cache its color
	if (newLigne) station->colors.push_back(stretch.ligne->color);
}

//---------------------------------------------------------------------
static void _register(std::list<Ligne> &lignes, Station *station, float curvature = 0.f)
{
	std::list<Stretch> &stretches = lignes.back().stretches;

	// first station
	if (stretches.empty())
	{
		stretches.emplace_back(station, nullptr, &lignes.back());
	}

	// second station
	else if (!stretches.back().b)
	{
		stretches.back().b = station;
		stretches.back().curvature = curvature;
	}

	// regular connection
	else
	{
		stretches.emplace_back(stretches.back().b, station, &lignes.back(), curvature);
		_register(stretches.back().a, stretches.back());
	}

	// register connection
	_register(station, stretches.back());
}

//---------------------------------------------------------------------
static void _register(std::list<Ligne> &lignes, Station *a, Station *b, float curvature = 0.f)
{
	// create ligne
	lignes.push_back(Ligne(White, true));

	// create
	lignes.back().stretches.emplace_back(a, b, &lignes.back(), curvature);

	// register
	a->stretches.push_back(&lignes.back().stretches.back());
	b->stretches.push_back(&lignes.back().stretches.back());
}

//---------------------------------------------------------------------
Metro::Metro()
{
	// stations
	_register(stations, new Station(Vec2(553.469f, 683.219f), L"Balard"));
	_register(stations, new Station(Vec2(574.545f, 704.020f), L"Lourmel"));
	_register(stations, new Station(Vec2(607.735f, 726.310f), L"Boucicaut"));
	_register(stations, new Station(Vec2(634.467f, 743.776f), L"Félix Faure"));
	_register(stations, new Station(Vec2(644.550f, 759.045f), L"Commerce"));
	_register(stations, new Station(Vec2(666.287f, 796.584f), L"La Motte-Picquet - Grenelle"));
	_register(stations, new Station(Vec2(713.402f, 845.182f), L"École Militaire"));
	_register(stations, new Station(Vec2(742.211f, 874.279f), L"La Tour Maubourg"));
	_register(stations, new Station(Vec2(764.243f, 917.351f), L"Invalides"));

	_register(stations, new Station(Vec2(616.736f, 860.327f), L"Champs de Mars - Bir-Hakeim"));
	_register(stations, new Station(Vec2(633.615f, 817.818f), L"Dupleix"));
	_register(stations, new Station(Vec2(691.339f, 788.218f), L"Cambronne"));
	_register(stations, new Station(Vec2(745.160f, 762.351f), L"Sèvres Lecourbe"));
	_register(stations, new Station(Vec2(755.106f, 740.054f), L"Pasteur"));
	_register(stations, new Station(Vec2(808.048f, 733.724f), L"Montparnasse Bienvenüe (S)"));
	_register(stations, new Station(Vec2(829.263f, 723.604f), L"Edgar Quinet"));
	_register(stations, new Station(Vec2(860.719f, 702.548f), L"Raspail"));
	_register(stations, new Station(Vec2(871.752f, 660.526f), L"Denfert Rochereau"));
	_register(stations, new Station(Vec2(897.263f, 654.560f), L"Saint-Jacques"));
	_register(stations, new Station(Vec2(944.303f, 636.827f), L"Glacière"));
	_register(stations, new Station(Vec2(978.409f, 623.478f), L"Corvisart"));
	_register(stations, new Station(Vec2(1010.26f, 633.376f), L"Place D'Italie"));
	_register(stations, new Station(Vec2(1044.56f, 655.314f), L"Nationale"));
	_register(stations, new Station(Vec2(1073.06f, 671.800f), L"Chevaleret"));
	_register(stations, new Station(Vec2(1114.19f, 696.415f), L"Quai de la gare"));

	_register(stations, new Station(Vec2(548.452f, 778.922f), L"Javel André Citroën"));
	_register(stations, new Station(Vec2(595.943f, 775.015f), L"Charles Michels"));
	_register(stations, new Station(Vec2(650.903f, 779.587f), L"Avenue Émile Zola"));
	_register(stations, new Station(Vec2(725.134f, 790.943f), L"Ségur"));
	_register(stations, new Station(Vec2(776.993f, 777.617f), L"Duroc"));
	_register(stations, new Station(Vec2(807.345f, 797.819f), L"Vaneau"));
	_register(stations, new Station(Vec2(838.218f, 817.842f), L"Sèvres Babylone"));
	_register(stations, new Station(Vec2(886.913f, 830.621f), L"Mabillon"));
	_register(stations, new Station(Vec2(912.753f, 825.236f), L"Odéon"));
	_register(stations, new Station(Vec2(949.292f, 815.325f), L"Cluny La Sorbonne"));
	_register(stations, new Station(Vec2(968.268f, 806.914f), L"Maubert Mutualité"));
	_register(stations, new Station(Vec2(985.843f, 778.266f), L"Cardinal Lemoine"));
	_register(stations, new Station(Vec2(1005.29f, 767.398f), L"Jussieu"));
	_register(stations, new Station(Vec2(1066.33f, 736.682f), L"Gare D'Austerlitz"));

	_register(stations, new Station(Vec2(611.032f, 649.782f), L"Porte de Versailles"));
	_register(stations, new Station(Vec2(659.553f, 690.978f), L"Convention"));
	_register(stations, new Station(Vec2(686.562f, 711.091f), L"Vaugirard"));
	_register(stations, new Station(Vec2(728.226f, 729.624f), L"Volontaires"));
	_register(stations, new Station(Vec2(786.556f, 756.347f), L"Falguière"));
	_register(stations, new Station(Vec2(825.241f, 748.655f), L"Montparnasse Bienvenüe (N)"));
	_register(stations, new Station(Vec2(851.494f, 760.513f), L"Notre-Dame-des-Champs"));
	_register(stations, new Station(Vec2(845.316f, 790.408f), L"Rennes"));
	_register(stations, new Station(Vec2(832.386f, 858.064f), L"Rue du Bac"));
	_register(stations, new Station(Vec2(816.295f, 885.361f), L"Solférino"));
	_register(stations, new Station(Vec2(800.779f, 909.498f), L"Assemblée Nationale"));

	_register(stations, new Station(Vec2(712.520f, 607.898f), L"Porte de Vanves"));
	_register(stations, new Station(Vec2(764.331f, 641.690f), L"Plaisance"));
	_register(stations, new Station(Vec2(788.509f, 661.601f), L"Pernety"));
	_register(stations, new Station(Vec2(814.516f, 702.847f), L"Gaîté"));
	_register(stations, new Station(Vec2(766.566f, 817.642f), L"Saint-François Xavier"));
	_register(stations, new Station(Vec2(769.411f, 863.967f), L"Varenne"));

	_register(stations, new Station(Vec2(834.781f, 568.156f), L"Porte D'Orléans"));
	_register(stations, new Station(Vec2(841.011f, 607.809f), L"Alésia"));
	_register(stations, new Station(Vec2(863.136f, 645.881f), L"Mouton Duvernet"));
	_register(stations, new Station(Vec2(852.074f, 736.824f), L"Vavin"));
	_register(stations, new Station(Vec2(838.712f, 777.051f), L"St.-Placide"));
	_register(stations, new Station(Vec2(863.280f, 817.566f), L"St.-Sulpice"));
	_register(stations, new Station(Vec2(879.134f, 841.195f), L"Saint Germain-des-Prés"));
	_register(stations, new Station(Vec2(952.271f, 838.908f), L"St-Michel - Notre-Dame"));

	_register(stations, new Station(Vec2(1030.06f, 677.994f), L"Campo Formio"));
	_register(stations, new Station(Vec2(1047.44f, 710.597f), L"St.-Marcel"));

	_register(stations, new Station(Vec2(1098.07f, 548.591f), L"Porte D'Ivry"));
	_register(stations, new Station(Vec2(1066.12f, 537.828f), L"Porte de Choisy"));
	_register(stations, new Station(Vec2(1037.15f, 529.813f), L"Porte d'Italie"));
	_register(stations, new Station(Vec2(1027.61f, 562.195f), L"Maison Blanche"));
	_register(stations, new Station(Vec2(1021.45f, 593.932f), L"Tolbiac"));
	_register(stations, new Station(Vec2(991.818f, 680.304f), L"Les Gobelins"));
	_register(stations, new Station(Vec2(987.948f, 720.435f), L"Censier Daubenton"));
	_register(stations, new Station(Vec2(990.800f, 743.046f), L"Place Monge"));

	_register(stations, new Station(Vec2(505.313f, 706.969f), L"Pont du Garigliano"));
	_register(stations, new Station(Vec2(684.724f, 920.430f), L"Pont de l'Alma"));
	_register(stations, new Station(Vec2(833.088f, 903.721f), L"Musée d'Orsay"));
	_register(stations, new Station(Vec2(1134.92f, 621.140f), L"Bibliotèque François Mitterand"));

	_register(stations, new Station(Vec2(915.156f, 542.594f), L"Cité Universitaire"));
	_register(stations, new Station(Vec2(901.809f, 715.278f), L"Port-Royal"));
	_register(stations, new Station(Vec2(919.689f, 776.638f), L"Luxembourg"));

	_register(stations, new Station(Vec2(976.908f, 543.696f), L"Poterne des Peupliers"));
	_register(stations, new Station(Vec2(954.953f, 534.264f), L"Stade Charléty"));
	_register(stations, new Station(Vec2(881.771f, 552.316f), L"Montsouris"));
	_register(stations, new Station(Vec2(795.253f, 581.101f), L"Jean Moulin"));
	_register(stations, new Station(Vec2(761.915f, 592.322f), L"Didot"));
	_register(stations, new Station(Vec2(688.245f, 616.716f), L"Brancion"));
	_register(stations, new Station(Vec2(655.069f, 627.612f), L"Georges Brassens"));
	_register(stations, new Station(Vec2(591.481f, 666.155f), L"Desnouettes"));

	_register(stations, new Station(Vec2(1085.08f, 598.517f), L"Olympiades"));

	// ligne 8
	lignes.push_back(Ligne(Color(0xA06CC4FFu)));
	_register(lignes, stations[L"Balard"]);
	_register(lignes, stations[L"Lourmel"], 2.f);
	_register(lignes, stations[L"Boucicaut"], 2.f);
	_register(lignes, stations[L"Félix Faure"], -1.f);
	_register(lignes, stations[L"Commerce"], -1.f);
	_register(lignes, stations[L"La Motte-Picquet - Grenelle"], -4.f);
	_register(lignes, stations[L"École Militaire"], 2.f);
	_register(lignes, stations[L"La Tour Maubourg"], -2.f);
	_register(lignes, stations[L"Invalides"], 2.f);

	// ligne 6
	lignes.push_back(Ligne(Color(0x2AA173FFu)));
	_register(lignes, stations[L"Champs de Mars - Bir-Hakeim"]);
	_register(lignes, stations[L"Dupleix"], -1.f);
	_register(lignes, stations[L"La Motte-Picquet - Grenelle"]);
	_register(lignes, stations[L"Cambronne"]);
	_register(lignes, stations[L"Sèvres Lecourbe"]);
	_register(lignes, stations[L"Pasteur"], -1.f);
	_register(lignes, stations[L"Montparnasse Bienvenüe (S)"], -1.f);
	_register(lignes, stations[L"Edgar Quinet"]);
	_register(lignes, stations[L"Raspail"]);
	_register(lignes, stations[L"Denfert Rochereau"], 4.f);
	_register(lignes, stations[L"Saint-Jacques"], -2.f);
	_register(lignes, stations[L"Glacière"]);
	_register(lignes, stations[L"Corvisart"]);
	_register(lignes, stations[L"Place D'Italie"]);
	_register(lignes, stations[L"Nationale"]);
	_register(lignes, stations[L"Chevaleret"]);
	_register(lignes, stations[L"Quai de la gare"]);

	// ligne 10
	lignes.push_back(Ligne(Color(0xB58900FFu)));
	_register(lignes, stations[L"Javel André Citroën"]);
	_register(lignes, stations[L"Charles Michels"], -1.f);
	_register(lignes, stations[L"Avenue Émile Zola"], -1.f);
	_register(lignes, stations[L"La Motte-Picquet - Grenelle"], 1.f);
	_register(lignes, stations[L"Ségur"], 1.f);
	_register(lignes, stations[L"Duroc"], -1.f);
	_register(lignes, stations[L"Vaneau"], -1.f);
	_register(lignes, stations[L"Sèvres Babylone"], 1.f);
	_register(lignes, stations[L"Mabillon"], 1.f);
	_register(lignes, stations[L"Odéon"], 1.f);
	_register(lignes, stations[L"Cluny La Sorbonne"], 1.f);
	_register(lignes, stations[L"Maubert Mutualité"], 1.f);
	_register(lignes, stations[L"Cardinal Lemoine"], 1.f);
	_register(lignes, stations[L"Jussieu"], -2.f);
	_register(lignes, stations[L"Gare D'Austerlitz"], 1.f);

	// ligne 12
	lignes.push_back(Ligne(Color(0x258F3EFFu)));
	_register(lignes, stations[L"Porte de Versailles"]);
	_register(lignes, stations[L"Convention"]);
	_register(lignes, stations[L"Vaugirard"]);
	_register(lignes, stations[L"Volontaires"]);
	_register(lignes, stations[L"Pasteur"]);
	_register(lignes, stations[L"Falguière"]);
	_register(lignes, stations[L"Montparnasse Bienvenüe (N)"]);
	_register(lignes, stations[L"Notre-Dame-des-Champs"]);
	_register(lignes, stations[L"Rennes"]);
	_register(lignes, stations[L"Sèvres Babylone"]);
	_register(lignes, stations[L"Rue du Bac"], -1.f);
	_register(lignes, stations[L"Solférino"], -1.f);
	_register(lignes, stations[L"Assemblée Nationale"], -1.f);

	// ligne 13
	lignes.push_back(Ligne(Color(0x2A9AA1FFu)));
	_register(lignes, stations[L"Porte de Vanves"]);
	_register(lignes, stations[L"Plaisance"]);
	_register(lignes, stations[L"Pernety"]);
	_register(lignes, stations[L"Gaîté"]);
	_register(lignes, stations[L"Montparnasse Bienvenüe (S)"]);
	_register(lignes, stations[L"Duroc"], -4.f);
	_register(lignes, stations[L"Saint-François Xavier"], 2.f);
	_register(lignes, stations[L"Varenne"]);
	_register(lignes, stations[L"Invalides"]);

	// ligne 4
	lignes.push_back(Ligne(Color(0x9236D3FFu)));
	_register(lignes, stations[L"Porte D'Orléans"]);
	_register(lignes, stations[L"Alésia"]);
	_register(lignes, stations[L"Mouton Duvernet"]);
	_register(lignes, stations[L"Denfert Rochereau"]);
	_register(lignes, stations[L"Raspail"], 4.f);
	_register(lignes, stations[L"Vavin"], -2.f);
	_register(lignes, stations[L"Montparnasse Bienvenüe (N)"], 2.f);
	_register(lignes, stations[L"St.-Placide"], 2.f);
	_register(lignes, stations[L"St.-Sulpice"], -6.f);
	_register(lignes, stations[L"Saint Germain-des-Prés"], 3.f);
	_register(lignes, stations[L"Odéon"], 5.f);
	_register(lignes, stations[L"St-Michel - Notre-Dame"], -2.f);

	// ligne 5
	lignes.push_back(Ligne(Color(0xCB4B16FFu)));
	_register(lignes, stations[L"Place D'Italie"]);
	_register(lignes, stations[L"Campo Formio"]);
	_register(lignes, stations[L"St.-Marcel"]);
	_register(lignes, stations[L"Gare D'Austerlitz"]);

	// ligne 7
	lignes.push_back(Ligne(Color(0xD33682FFu)));
	_register(lignes, stations[L"Porte D'Ivry"]);
	_register(lignes, stations[L"Porte de Choisy"], -4.f);
	_register(lignes, stations[L"Porte d'Italie"], 4.f);
	_register(lignes, stations[L"Maison Blanche"], 2.f);
	_register(lignes, stations[L"Tolbiac"]);
	_register(lignes, stations[L"Place D'Italie"]);
	_register(lignes, stations[L"Les Gobelins"]);
	_register(lignes, stations[L"Censier Daubenton"]);
	_register(lignes, stations[L"Place Monge"]);
	_register(lignes, stations[L"Jussieu"]);

	// RER C
	lignes.push_back(Ligne(Color(0xB59100FFu)));
	_register(lignes, stations[L"Pont du Garigliano"]);
	_register(lignes, stations[L"Javel André Citroën"]);
	_register(lignes, stations[L"Champs de Mars - Bir-Hakeim"]);
	_register(lignes, stations[L"Pont de l'Alma"]);
	_register(lignes, stations[L"Invalides"]);
	_register(lignes, stations[L"Musée d'Orsay"], 5.f);
	_register(lignes, stations[L"St-Michel - Notre-Dame"]);
	_register(lignes, stations[L"Gare D'Austerlitz"]);
	_register(lignes, stations[L"Bibliotèque François Mitterand"]);

	// RER B
	lignes.push_back(Ligne(Color(0x268BD2FFu)));
	_register(lignes, stations[L"Cité Universitaire"]);
	_register(lignes, stations[L"Denfert Rochereau"]);
	_register(lignes, stations[L"Port-Royal"]);
	_register(lignes, stations[L"Luxembourg"]);
	_register(lignes, stations[L"St-Michel - Notre-Dame"], 5.f);

	// T3
	lignes.push_back(Ligne(Color(0xCB4B16FFu)));
	_register(lignes, stations[L"Porte D'Ivry"]);
	_register(lignes, stations[L"Porte de Choisy"], 4.f);
	_register(lignes, stations[L"Porte d'Italie"], -4.f);
	_register(lignes, stations[L"Poterne des Peupliers"], -1.f);
	_register(lignes, stations[L"Stade Charléty"], -1.f);
	_register(lignes, stations[L"Cité Universitaire"], 1.f);
	_register(lignes, stations[L"Montsouris"], 1.f);
	_register(lignes, stations[L"Porte D'Orléans"], 1.f);
	_register(lignes, stations[L"Jean Moulin"], 1.f);
	_register(lignes, stations[L"Didot"], 1.f);
	_register(lignes, stations[L"Porte de Vanves"], 1.f);
	_register(lignes, stations[L"Brancion"], 1.f);
	_register(lignes, stations[L"Georges Brassens"], 1.f);
	_register(lignes, stations[L"Porte de Versailles"], 1.f);
	_register(lignes, stations[L"Desnouettes"], -2.f);
	_register(lignes, stations[L"Balard"], 2.f);
	_register(lignes, stations[L"Pont du Garigliano"], -1.f);

	// ligne 14
	lignes.push_back(Ligne(Color(0x6528B8FFu)));
	_register(lignes, stations[L"Bibliotèque François Mitterand"]);
	_register(lignes, stations[L"Olympiades"]);

	// piéton
	_register(lignes, stations[L"Montparnasse Bienvenüe (N)"], stations[L"Montparnasse Bienvenüe (S)"]);
	_register(lignes, stations[L"Campo Formio"], stations[L"Nationale"]);
	_register(lignes, stations[L"Vavin"], stations[L"Notre-Dame-des-Champs"]);
	_register(lignes, stations[L"Assemblée Nationale"], stations[L"Invalides"], 1.f);
	_register(lignes, stations[L"Tolbiac"], stations[L"Corvisart"], 2.f);
	_register(lignes, stations[L"Porte D'Ivry"], stations[L"Olympiades"], -1.f);
	_register(lignes, stations[L"Raspail"], stations[L"Port-Royal"], 2.f);
	_register(lignes, stations[L"Sèvres Lecourbe"], stations[L"Duroc"], 2.f);
	_register(lignes, stations[L"Félix Faure"], stations[L"Vaugirard"], -4.f);
	_register(lignes, stations[L"Solférino"], stations[L"Musée d'Orsay"], -2.f);
}

//---------------------------------------------------------------------
void Game::prepareWave()
{
	// spawn cops
	if (1 <= score && score <= 3)
	{
		// create cops
		leMetro.forEachLigne([&] (Ligne *ligne)
		{
			// express lignes don't have cops
			if (ligne->isExpress) return true;

			// limit cops per ligne
			if (score == 2 && ligne->stretches.size() + 1 <= 5 ||
				score == 3 && ligne->stretches.size() + 1 <= 10)
				return true;

			// random station
			auto n = ligne->stretches.size();
			auto it = ligne->stretches.begin();
			std::advance(it, (unsigned)(random::unif() * n));

			// check station (don't spawn it on the player)
			Station *station = it->b;

			if (station == player->getStation())
				station = it->a;

			// create cop
			voyageurs.push_back(new Cop(Direction(ligne, score % 2 == 1), station));
			return true;
		});
	}

	// accelerate trains
	if (score == 4)
		Metro::timeFactor = 0.75f;
	else if (score == 5)
		Metro::timeFactor = 0.5f;
}
