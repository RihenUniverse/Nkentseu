// =============================================================================
// NkMatGraphDemo — le graphe de materiaux REND VRAIMENT, et on le prouve par des
//                  valeurs calculables, pas par une image.
// =============================================================================
// POURQUOI CE BANC EXISTE, alors que NkMatGraphCheck compile deja sur quatre
// backends : compiler n'est pas rendre. Un shader peut traverser GL, Vulkan,
// DX11 et DX12 sans qu'aucune des valeurs declarees dans le graphe n'arrive au
// pixel — un defaut de prise perdu, un canal permute, un facteur de melange
// ignore. Rien de tout cela n'empeche la compilation.
//
// ⚠️ ET CE BANC NE JUGE PAS PAR L'IMAGE. C'est la lecon la plus chere de la
// nuit du 21 au 22/08 : en verifiant qu'un temoin par signature d'image saurait
// attraper une erreur de binding, on a ecrit VOLONTAIREMENT un descripteur sur
// un binding absent du layout. Resultat : aucune erreur, aucun journal, et les
// cinq signatures d'image IDENTIQUES. **Une ressource qui n'arrive pas ne change
// pas l'image tant que personne ne la lit.** Un temoin par capture est donc
// structurellement aveugle a toute une classe de defauts.
//
// CE QU'ON MESURE A LA PLACE : la DIFFERENCE ENTRE CANAUX d'un meme pixel, et
// la difference entre deux rendus. Les deux sont calculables A PARTIR DU GRAPHE,
// sans rien savoir du modele d'eclairage — et c'est tout l'interet.
//
//   Le shader engendre finit par :
//       fragColor = diffuse + specular + emission
//   Avec un noeud Emission, le graphe impose albedo = 0 et metallic = 0. Donc :
//       diffuse   = albedo * (...)              = 0
//       specColor = mix(vec3(1), albedo, 0)     = vec3(1)
//       specular  = vec3(1) * spec * 0.3        = ACHROMATIQUE
//   **Tout ce qui n'est pas l'emission est donc gris.** Quel que soit
//   l'eclairage, quelle que soit la normale, quel que soit `spec` :
//       pixel = (S, S, S) + emission,  S inconnu mais IDENTIQUE aux trois canaux
//   La soustraction de deux canaux ELIMINE S. Ce qui reste est exactement ce que
//   le graphe a declare, et rien d'autre.
//
// On rend donc en RGBA8 **UNORM** (lineaire, pas sRGB) : une emission de
// 128/255 dans le graphe doit rendre un ecart de canal de **128, exactement**.
// C'est une valeur attendue calculable, pas une ressemblance.
//
// SANS FENETRE : device DX11 headless (« Mode headless (pas de HWND) », motif
// deja prouve par NkGpuProbe) + `Tools/Offscreen`, REUTILISE tel quel. Aucune
// ligne de NkOffscreenTarget n'est modifiee : on se sert de `ReadbackPixels`,
// jamais de `Capture(path)` — un PNG serait precisement le temoin aveugle.
//
// APPLICATION et non `tests/` : la politique du workspace desactive l'execution
// des tests unitaires ; un banc qui doit prouver quelque chose est une
// application console.
// =============================================================================
#include "NKRHI/Commands/NkICommandBuffer.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkIDevice.h"

#include "NKRenderer/Core/NkTextureLibrary.h"
#include "NKRenderer/Materials/Graph/NkMatGraphCompile.h"
#include "NKRenderer/Shader/NkShaderLibrary.h"
#include "NKRenderer/Tools/Offscreen/NkOffscreenTarget.h"

// ── POURQUOI CE BANC N'IMPRIME PLUS AVEC printf ─────────────────────────────
// Rodolf, 2026-08-22 (et c'est la deuxieme fois) : « ne pas utiliser directement
// printf, le systeme definit des loggers, pourquoi ne pas les utiliser ? »
//
// ⚠️ ET PASSER A `Infof` N'AURAIT RIEN CORRIGE. `Infof` prend une chaine de
// format et des arguments variadiques NON TYPES — la meme mecanique que printf.
// Le defaut mesure la meme nuit par l'agent NK3DModeler — un printf reclamant
// sept `%u` pour six arguments, le septieme lisant la pile et affichant
// « nonmanif=18 » sur un cube parfaitement manifold — se reproduit a l'identique
// avec `Infof`. Rien ne plante, rien n'avertit, ET LE NOMBRE EST CREDIBLE.
//
// Seule la forme POSITIONNELLE change la nature du probleme : `NkFormat("{0}")`
// capture les arguments PAR LEUR TYPE, jamais reinterpretes. S'il en manque un,
// on obtient un TROU VISIBLE, pas un entier plausible. Pour un banc dont toute
// la valeur tient dans les nombres qu'il imprime, c'est la seule propriete qui
// compte : un trou se voit ; un « 18 » plausible se recopie dans un rapport,
// puis dans une ROADMAP, puis dans une decision.
//
//   `-Werror=format` (pose dans le .jenga) DETECTE le desalignement.
//   La forme `{0}` le rend IRREPRESENTABLE. On garde les deux : le drapeau est
//   un filet pour ce qui resterait, la forme est la solution.
//
// Le motif de journal est « %v » : le message SEUL, sans horodatage ni fichier.
// La sortie d'un banc EST son resultat ; une decoration la rendrait illisible et
// instable d'une execution a l'autre.
//
// ⚠️ Et les chaines IMPRIMEES sont en ASCII : le puits console ne transporte pas
// l'UTF-8 (mesure — « — » sortait « - », les guillemets sortaient « ? »). Mieux
// vaut ecrire ce qui sera lu que laisser des caracteres se perdre en chemin. Les
// commentaires, eux, gardent leur typographie : ils ne sont jamais imprimes.
#include "NKContainers/String/NkFormat.h"
#include "NKLogger/NkLog.h"

// NkShaderStage existe DEUX FOIS : celui du RHI (bitmask, = NkSLStage) et
// celui de renderer/NkShaderBackend.h. Sans cet alias, toute mention du nom
// est ambigue. Meme levee que NkMaterialSystem.cpp.
using RHIStage = ::nkentseu::NkShaderStage;

using namespace nkentseu;
using namespace nkentseu::graph;
using namespace nkentseu::renderer;
using namespace nkentseu::renderer::matgraph;

// ── comptage des cas ─────────────────────────────────────────────────────────
static uint32 gCas = 0;
static uint32 gEchecs = 0;

static void Cas(const char *nom, bool ok, const NkString &detail) {
	++gCas;
	if (!ok)
		++gEchecs;
	logger.Info("{0:<32} {1} | {2}", NkString(nom), NkString(ok ? "OK  " : "ECHEC"), detail);
}

// ── L'UBO camera, tel que le shader engendre le declare ──────────────────────
// La disposition doit correspondre EXACTEMENT au bloc emis par
// NkMatCompileToNkSL. Un decalage ici ne provoquerait aucune erreur : le shader
// lirait simplement d'autres octets, et `camPos` vaudrait n'importe quoi. C'est
// la meme famille de defaut silencieux que le binding non declare.
struct DemoCameraUBO {
		float32 view[16] = {};
		float32 proj[16] = {};
		float32 viewProj[16] = {};
		float32 invViewProj[16] = {};
		float32 camPos[4] = {0.f, 0.f, 5.f, 1.f};
		float32 camDir[4] = {0.f, 0.f, -1.f, 0.f};
		float32 viewport[2] = {64.f, 64.f};
		float32 time = 0.f;
		float32 deltaTime = 0.f;
		float32 iblStrength = 1.f;
		float32 _pad[3] = {};
};

// ── Un sommet du triangle plein ecran ────────────────────────────────────────
// On FOURNIT les varyings au lieu de les calculer : la position est deja en
// espace de clip, la normale et la couleur sont constantes. Le shader engendre
// n'a alors plus aucune entree cachee — tout ce qu'il lit vient soit du graphe,
// soit d'ici.
struct DemoVertex {
		float32 pos[3];
};

static const char *const kVertexNkSL = R"NKSL(
// Vertex minimal du banc : il ne transforme RIEN et ne lit qu'UN attribut.
//
// ⚠️ UN SEUL ATTRIBUT, ET IL S'APPELLE `aPos`. Le generateur HLSL deduit la
// SEMANTIQUE d'un attribut de son NOM DE VARIABLE (table dans
// NkSLCodeGenHLSLStructs.cpp) : « apos » y donne POSITION. « acolor » et « auv »
// ne figurent dans aucune entree et retomberaient sur TEXCOORD<location> — le
// layout declare cote C++ ne correspondrait plus au shader, et rien ne le
// dirait. On synthetise donc les autres varyings ici : elles deviennent des
// constantes connues de l'appelant, ce qui est justement ce qu'on veut.
@location(0) in vec3 aPos;

@location(0) out vec3 vWorldPos;
@location(1) out vec3 vNormal;
@location(2) out vec2 vUV;
@location(3) out vec4 vColor;

@stage(vertex)
@entry
void main() {
    vWorldPos   = aPos;
    vNormal     = vec3(0.0, 0.0, 1.0);
    vUV         = vec2(0.5, 0.5);
    vColor      = vec4(1.0, 1.0, 1.0, 1.0);
    gl_Position = vec4(aPos, 1.0);
}
)NKSL";

// ── Le contexte de rendu, monte une seule fois ───────────────────────────────
struct DemoContexte {
		NkIDevice *device = nullptr;
		NkTextureLibrary texLib;
		NkShaderLibrary shaders;
		NkOffscreenTarget cible;
		NkBufferHandle vbo;
		NkBufferHandle ubo;
		NkDescSetHandle setLayout;
		NkDescSetHandle set;
		uint32 largeur = 64, hauteur = 64;

		bool Monter();
		void Demonter();
		// Compile le NkSL donne, rend un triangle plein ecran, et rend la couleur
		// du pixel CENTRAL. `false` si quoi que ce soit echoue.
		bool RendreEtLire(const NkString &fragmentNkSL, const char *nom, uint8 rgba[4]);
};

bool DemoContexte::Monter() {
	// DX11 headless : pas de surface -> pas de swapchain. Motif deja prouve par
	// NkGpuProbe, et le device le dit lui-meme dans son journal.
	NkDeviceInitInfo di;
	di.api = NkGraphicsApi::NK_GFX_API_DX11;
	di.width = 0;
	di.height = 0;
	device = NkDeviceFactory::Create(di);
	if (!device || !device->IsValid()) {
		logger.Info("device DX11 headless : KO");
		return false;
	}
	if (texLib.Init(device, nullptr) != NkRResult::NK_OK) {
		logger.Info("NkTextureLibrary : KO");
		return false;
	}
	if (!shaders.Init(device, device->GetApi(), /*useNkSL=*/true)) {
		logger.Info("NkShaderLibrary : KO");
		return false;
	}

	NkOffscreenDesc od;
	od.width = largeur;
	od.height = hauteur;
	// PAS DE PROFONDEUR : un seul triangle, rien a trier. Une passe sans
	// attachement de profondeur evite d'avoir a accorder l'etat du pipeline avec
	// la passe — un desaccord la se paierait en pixels manquants sans message.
	od.hasDepth = false;
	// ⚠️ UNORM, PAS sRGB. Avec sRGB, l'ecart de canal ne vaudrait plus la valeur
	// declaree dans le graphe mais son encodage — l'attendu cesserait d'etre
	// calculable, et on retomberait sur « ca ressemble ».
	od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
	od.readable = true;
	od.readback = true;
	od.name = NkString("NkMatGraphDemo");
	if (!cible.Init(device, &texLib, od)) {
		logger.Info("NkOffscreenTarget : KO");
		return false;
	}

	// Triangle plein ecran (3 sommets, pas de quad : un triangle suffit et evite
	// la couture diagonale au centre, la ou l'on echantillonne).
	const DemoVertex sommets[3] = {
		{{-1.f, -1.f, 0.f}},
		{{3.f, -1.f, 0.f}},
		{{-1.f, 3.f, 0.f}},
	};
	vbo = device->CreateBuffer(NkBufferDesc::Vertex(sizeof(sommets), sommets));
	if (!vbo.IsValid()) {
		logger.Info("vertex buffer : KO");
		return false;
	}

	DemoCameraUBO cam;
	ubo = device->CreateBuffer(NkBufferDesc::Uniform(sizeof(DemoCameraUBO)));
	if (!ubo.IsValid()) {
		logger.Info("uniform buffer : KO");
		return false;
	}
	device->WriteBuffer(ubo, &cam, sizeof(cam));

	NkDescriptorSetLayoutDesc ld;
	ld.Add(0, NkDescriptorType::NK_UNIFORM_BUFFER, RHIStage::NK_ALL_GRAPHICS);
	setLayout = device->CreateDescriptorSetLayout(ld);
	set = device->AllocateDescriptorSet(setLayout);
	NkDescriptorWrite w{};
	w.set = set;
	w.binding = 0;
	w.type = NkDescriptorType::NK_UNIFORM_BUFFER;
	w.buffer = ubo;
	w.bufferRange = sizeof(DemoCameraUBO);
	device->UpdateDescriptorSets(&w, 1);
	return true;
}

void DemoContexte::Demonter() {
	cible.Shutdown();
	shaders.Shutdown();
	texLib.Shutdown();
	if (device)
		NkDeviceFactory::Destroy(device);
}

bool DemoContexte::RendreEtLire(const NkString &fragmentNkSL, const char *nom, uint8 rgba[4]) {
	::nkentseu::NkShaderHandle prog = shaders.CompileVF(NkString(kVertexNkSL), fragmentNkSL, NkString(nom));
	::nkentseu::NkShaderHandle rhi = shaders.GetRHIHandle(prog);
	if (!rhi.IsValid())
		return false;

	NkGraphicsPipelineDesc pd;
	pd.shader = rhi;
	pd.vertexLayout.AddBinding(0, (uint32)sizeof(DemoVertex))
		.AddAttribute(0, 0, NkGPUFormat::NK_RGB32_FLOAT, 0, "POSITION", 0);
	// Aucun cull : le sens d'enroulement du triangle plein ecran n'a aucune
	// importance ici, et un cull mal oriente rendrait une image NOIRE sans le
	// moindre message — encore un echec silencieux.
	pd.rasterizer.cullMode = NkCullMode::NK_NONE;
	pd.depthStencil = NkDepthStencilDesc::NoDepth();
	pd.renderPass = cible.GetRP();
	pd.descriptorSetLayouts.PushBack(setLayout);
	pd.debugName = nom;
	NkPipelineHandle pipe = device->CreateGraphicsPipeline(pd);
	if (!pipe.IsValid())
		return false;

	NkICommandBuffer *cmd = device->CreateCommandBuffer();
	if (!cmd || !cmd->Begin())
		return false;
	// Fond MAGENTA opaque, jamais noir. Si le tracé échouait en silence, un fond
	// noir se confondrait avec « le graphe a rendu du noir » — la couleur de
	// repli doit être une couleur que le graphe ne peut pas produire.
	// BeginCapture pose lui-meme la barriere de texture, l'effacement, la passe,
	// le viewport et le ciseau. EndCapture ferme la passe ET remet la texture en
	// SHADER_READ — etat que ReadbackPixels SUPPOSE. Ecrire la passe a la main
	// laisserait la texture dans le mauvais etat, et le readback lirait du vide
	// sans se plaindre.
	cible.BeginCapture(cmd, /*clearColor=*/true, NkVec4f{1.f, 0.f, 1.f, 1.f}, /*clearDepth=*/false);
	cmd->BindGraphicsPipeline(pipe);
	cmd->BindDescriptorSet(set, 0);
	cmd->BindVertexBuffer(0, vbo);
	cmd->Draw(3);
	cible.EndCapture(cmd);
	cmd->End();
	device->Submit(&cmd, 1);
	device->WaitIdle();

	// ReadbackPixels fait lui-meme la copie GPU, la soumission, l'attente et le
	// retournement selon le backend. On lit tout, puis on prend le pixel CENTRAL.
	const uint32 n = largeur * hauteur * 4u;
	NkVector<uint8> px;
	px.Resize(n);
	if (!cible.ReadbackPixels(px.Data(), largeur * 4u))
		return false;
	const uint32 c = ((hauteur / 2u) * largeur + (largeur / 2u)) * 4u;
	rgba[0] = px[c + 0];
	rgba[1] = px[c + 1];
	rgba[2] = px[c + 2];
	rgba[3] = px[c + 3];
	device->DestroyPipeline(pipe);
	return true;
}

// ── Les graphes du banc ──────────────────────────────────────────────────────
// L'emission vaut 128/255 EXACTEMENT. Ce n'est pas une coquetterie : en RGBA8
// lineaire, une valeur de 128/255 rend un octet de 128, et l'ecart entre le
// canal emissif et un canal voisin doit donc valoir 128 SANS arrondi. Un
// attendu qui tomberait entre deux octets obligerait a une tolerance, et une
// tolerance cache exactement ce qu'on cherche.
static const float32 kE = 128.f / 255.f;
static const int32 kEcartAttendu = 128;

// Emission pure sur un canal : color = (E,0,0) ou (0,E,0), strength = 1.
static NkNodeId AjouteEmission(NkNodeGraph &g, const NkMatTypes &t, uint32 canal) {
	const NkNodeId n = NkMatAddNode(g, NK_MN_EMISSION);
	float32 c[3] = {0.f, 0.f, 0.f};
	c[canal] = kE;
	g.SetSocketDefault(n, "color", NkSocketDir::Input, NkValueVec(t.color, c, 3));
	g.SetSocketDefault(n, "strength", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	return n;
}

// Un graphe : une emission (canal donne) vers la sortie.
static bool GrapheEmission(uint32 canal, NkString &out) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId sortie = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId e = AjouteEmission(g, t, canal);
	g.Connect(e, "emission", sortie, "surface");
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	out = r.source;
	return r.ok;
}

// Le graphe de Rodolf : DEUX emissions melangees par un Mix Shader.
static bool GrapheMelange(float32 fac, NkString &out) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId sortie = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId mix = NkMatAddNode(g, NK_MN_MIX_SHADER);
	const NkNodeId rouge = AjouteEmission(g, t, 0);
	const NkNodeId vert = AjouteEmission(g, t, 1);
	g.Connect(rouge, "emission", mix, "shader1");
	g.Connect(vert, "emission", mix, "shader2");
	g.Connect(mix, "shader", sortie, "surface");
	g.SetSocketDefault(mix, "fac", NkSocketDir::Input, NkValueReal(t.real, fac));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	out = r.source;
	return r.ok;
}

int main() {
	// ⚠️ `Pattern()` est GLOBAL ET PERSISTANT : il modifie l'instance de journal
	// du PROCESSUS, pas l'appel. On le pose donc UNE FOIS ici, et pas a chaque
	// ligne — sinon chaque composant du moteur qui journalise ensuite herite
	// silencieusement du motif du banc. Consequence assumee : les lignes du
	// moteur perdent aussi leur horodatage dans ce processus ; elles restent
	// reconnaissables a leur crochet ouvrant, et se filtrent par « ^\[ ».
	logger.Pattern("%v");
	logger.Info("== NkMatGraphDemo - le graphe REND, et l'attendu est calculable ==");
	logger.Info("   DX11 headless, 64x64 hors-ecran, RGBA8 UNORM (lineaire).");
	logger.Info("   On ne compare AUCUNE image : on mesure des ecarts entre canaux,");
	logger.Info("   que le modele d'eclairage ne peut ni creer ni masquer.\n");

	DemoContexte ctx;
	if (!ctx.Monter()) {
		logger.Info("\n-- montage impossible : AUCUN cas n'a tourne. C'est un ECHEC, pas un saut. --");
		return 1;
	}

	uint8 rouge[4] = {}, vert[4] = {}, mix0[4] = {}, mix1[4] = {}, mixMoitie[4] = {};
	NkString src;
	bool ok = true;

	// ── 1. emission rouge ────────────────────────────────────────────────
	ok = GrapheEmission(0, src) && ctx.RendreEtLire(src, "matgraph_rouge", rouge);
	{
		// DISCRIMINE : V == B elimine le terme achromatique ; l'ecart R-V est
		// exactement ce que le graphe a declare. Un defaut de prise perdu
		// donnerait 0, un canal permute mettrait l'ecart ailleurs.
		const int32 ecart = (int32)rouge[0] - (int32)rouge[1];
		NkString d;
		d = NkFormat("pixel=({0},{1},{2}) | V==B : {3} | ecart R-V={4} (attendu {5}, sans tolerance)", rouge[0], rouge[1], rouge[2], rouge[1] == rouge[2] ? "oui" : "NON", ecart, kEcartAttendu);
		Cas("rendu/emission-rouge", ok && rouge[1] == rouge[2] && ecart == kEcartAttendu, d);
	}

	// ── 2. emission verte ────────────────────────────────────────────────
	ok = GrapheEmission(1, src) && ctx.RendreEtLire(src, "matgraph_vert", vert);
	{
		// LE TEMOIN DE L'AUTRE CANAL. Sans lui, un shader qui ecrirait toujours
		// dans le rouge passerait le cas precedent.
		const int32 ecart = (int32)vert[1] - (int32)vert[0];
		NkString d;
		d = NkFormat("pixel=({0},{1},{2}) | R==B : {3} | ecart V-R={4} (attendu {5})", vert[0], vert[1], vert[2], vert[0] == vert[2] ? "oui" : "NON", ecart, kEcartAttendu);
		Cas("rendu/emission-verte", ok && vert[0] == vert[2] && ecart == kEcartAttendu, d);
	}

	// ── 3. le terme achromatique est bien LE MEME dans les deux rendus ───
	{
		// DISCRIMINE : c'est ce qui autorise a comparer les cas entre eux. Les
		// deux graphes ne different QUE par l'emission ; leur plancher gris doit
		// donc etre identique au bit. S'il ne l'etait pas, toutes les
		// comparaisons croisees qui suivent seraient sans valeur.
		NkString d;
		d = NkFormat("plancher gris rouge={0} vert={1} (identiques attendus)", rouge[2], vert[2]);
		Cas("rendu/plancher-achromatique-stable", rouge[2] == vert[2], d);
	}

	// ── 4. Mix Shader, facteur 0 puis 1 ──────────────────────────────────
	ok = GrapheMelange(0.f, src) && ctx.RendreEtLire(src, "matgraph_mix0", mix0);
	{
		// fac=0 doit rendre EXACTEMENT le premier shader. Comparaison des trois
		// canaux : un melange qui prendrait la moyenne quel que soit le facteur
		// donnerait un rouge a moitie, ce qu'un simple « R > V » laisserait
		// passer.
		const bool identique = mix0[0] == rouge[0] && mix0[1] == rouge[1] && mix0[2] == rouge[2];
		NkString d;
		d = NkFormat("pixel=({0},{1},{2}) | identique au rendu rouge seul : {3}", mix0[0], mix0[1], mix0[2], identique ? "oui" : "NON");
		Cas("rendu/melange-fac-0-donne-le-premier", ok && identique, d);
	}

	ok = GrapheMelange(1.f, src) && ctx.RendreEtLire(src, "matgraph_mix1", mix1);
	{
		const bool identique = mix1[0] == vert[0] && mix1[1] == vert[1] && mix1[2] == vert[2];
		NkString d;
		d = NkFormat("pixel=({0},{1},{2}) | identique au rendu vert seul : {3}", mix1[0], mix1[1], mix1[2], identique ? "oui" : "NON");
		Cas("rendu/melange-fac-1-donne-le-second", ok && identique, d);
	}

	// ── 5. Mix Shader a mi-chemin — LE CAS QUE RODOLF A DEMANDE ──────────
	ok = GrapheMelange(0.5f, src) && ctx.RendreEtLire(src, "matgraph_mixmoitie", mixMoitie);
	{
		// TROIS choses a la fois, et il en faut trois :
		//   R == V           : les deux emissions arrivent a parts egales ;
		//   ecart == 64      : la MOITIE de 128, calcule depuis le graphe ;
		//   plancher inchange: le terme achromatique n'a pas bouge, donc l'ecart
		//                      mesure bien l'emission et rien d'autre.
		// Un melange qui ignorerait le facteur rendrait 128 ; un melange
		// n'agissant que sur une composante casserait R == V.
		const int32 ecartR = (int32)mixMoitie[0] - (int32)mixMoitie[2];
		const int32 ecartV = (int32)mixMoitie[1] - (int32)mixMoitie[2];
		const bool plancher = mixMoitie[2] == rouge[2];
		NkString d;
		d = NkFormat("pixel=({0},{1},{2}) | R==V : {3} | ecarts R-B={4} V-B={5} (attendu {6}) | plancher intact : {7}", mixMoitie[0], mixMoitie[1], mixMoitie[2], mixMoitie[0] == mixMoitie[1] ? "oui" : "NON", ecartR, ecartV, kEcartAttendu / 2, plancher ? "oui" : "NON");
		Cas("rendu/melange-a-mi-chemin",
			ok && mixMoitie[0] == mixMoitie[1] && ecartR == kEcartAttendu / 2 && ecartV == kEcartAttendu / 2 &&
				plancher,
			d);
	}

	// ── 6. le fond de repli n'a jamais ete lu ────────────────────────────
	{
		// ⚠️ LE CAS QUI EMPECHE DE SE REJOUIR TROP VITE. Si le trace avait
		// echoue sans le dire, on lirait le MAGENTA d'effacement. Comme il est
		// impossible a produire par ces graphes (B vaut le plancher gris, jamais
		// 255), sa presence signerait un rendu qui n'a pas eu lieu.
		const bool magenta = rouge[0] == 255 && rouge[1] == 0 && rouge[2] == 255;
		NkString d;
		d = NkFormat("premier pixel=({0},{1},{2}) | magenta d'effacement lu : {3}", rouge[0], rouge[1], rouge[2], magenta ? "OUI -- le trace n'a pas eu lieu" : "non");
		Cas("rendu/le-trace-a-bien-eu-lieu", !magenta, d);
	}

	ctx.Demonter();
	logger.Info("\n-- {0} cas, {1} echec(s) --", gCas, gEchecs);
	return gEchecs == 0 ? 0 : 1;
}
