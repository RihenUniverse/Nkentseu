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
    // L'UV VARIE avec la position : sans cela toute derivee d'ecran vaut
    // ZERO, et `Bump` comme la base tangente de `Normal Map` ne pourraient
    // etre mesures que sur leur absence d'effet. Les cas d'emission n'en
    // sont pas touches : ils ne lisent pas l'UV.
    vUV         = aPos.xy * 0.5 + 0.5;
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

// Le graphe qui prouve qu'une PROPRIETE ENUMEREE change le pixel :
//     RGB(rouge) --color1--> Mix Color --color--> Emission --> sortie
//     RGB(vert)  --color2-->
// L'emission garde albedo = 0 et metallic = 0, donc l'invariant achromatique
// tient : tout ce qui n'est pas l'emission reste gris, et la soustraction de
// deux canaux elimine ce gris inconnu.
static bool GrapheMelangeCouleur(const char *operation, float32 fac, NkString &out) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId sortie = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId emis = NkMatAddNode(g, NK_MN_EMISSION);
	const NkNodeId mix = NkMatAddNode(g, NK_MN_MIX_COLOR);
	const NkNodeId rouge = NkMatAddNode(g, NK_MN_RGB);
	const NkNodeId vert = NkMatAddNode(g, NK_MN_RGB);
	g.Connect(emis, "emission", sortie, "surface");
	g.Connect(mix, "color", emis, "color");
	g.Connect(rouge, "color", mix, "color1");
	g.Connect(vert, "color", mix, "color2");
	const float32 cr[3] = {kE, 0.f, 0.f};
	const float32 cv[3] = {0.f, kE, 0.f};
	g.SetProp(rouge, NK_MPROP_COLOR, NkValueVec(t.color, cr, 3));
	g.SetProp(vert, NK_MPROP_COLOR, NkValueVec(t.color, cv, 3));
	g.SetProp(mix, NK_MPROP_OPERATION, NkValueText(t.real, operation));
	g.SetSocketDefault(mix, "fac", NkSocketDir::Input, NkValueReal(t.real, fac));
	g.SetSocketDefault(emis, "strength", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	out = r.source;
	return r.ok;
}

// Le graphe qui prouve qu'une charge utile VARIABLE atteint le GPU :
//     Value(fac) --> ColorRamp --> Emission --> sortie
// La rampe recoit N arrets ; l'emission garde albedo = 0 et metallic = 0, donc
// l'invariant achromatique tient et la soustraction de deux canaux elimine le
// terme d'eclairage inconnu.
static bool GrapheRampe(const float32 *arrets, uint32 nbReels, float32 fac, NkString &out) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId sortie = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId emis = NkMatAddNode(g, NK_MN_EMISSION);
	const NkNodeId ramp = NkMatAddNode(g, NK_MN_COLOR_RAMP);
	const NkNodeId val = NkMatAddNode(g, NK_MN_VALUE);
	g.Connect(emis, "emission", sortie, "surface");
	g.Connect(ramp, "color", emis, "color");
	g.Connect(val, "value", ramp, "fac");
	g.SetProp(ramp, NK_MPROP_STOPS, NkValueVec(t.ramp, arrets, nbReels));
	g.SetProp(ramp, NK_MPROP_INTERP, NkValueText(t.ramp, "lineaire"));
	g.SetProp(val, NK_MPROP_VALUE, NkValueReal(t.real, fac));
	g.SetSocketDefault(emis, "strength", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	out = r.source;
	return r.ok;
}

// Un Principled gris dont la NORMALE est imposee directement, en passant par un
// noeud `RGB` : c'est l'ETALON. Il donne, avec le meme eclairage et le meme
// shader, la couleur que produit une normale connue — sans que le banc ait a
// reimplanter la moindre ligne d'ombrage.
static bool GrapheNormaleImposee(float32 nx, float32 ny, float32 nz, NkString &out) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId sortie = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId rgb = NkMatAddNode(g, NK_MN_RGB);
	g.Connect(bsdf, "bsdf", sortie, "surface");
	g.Connect(rgb, "color", bsdf, "normal");
	const float32 n[3] = {nx, ny, nz};
	g.SetProp(rgb, NK_MPROP_COLOR, NkValueVec(t.color, n, 3));
	const float32 gris[3] = {0.5f, 0.5f, 0.5f};
	g.SetSocketDefault(bsdf, "base_color", NkSocketDir::Input, NkValueVec(t.color, gris, 3));
	g.SetSocketDefault(bsdf, "metallic", NkSocketDir::Input, NkValueReal(t.real, 0.f));
	g.SetSocketDefault(bsdf, "roughness", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	out = r.source;
	return r.ok;
}

// Le meme Principled gris, mais dont la normale vient d'un `Bump` alimente par
// la coordonnee de texture : la hauteur croit lineairement a l'ecran, donc son
// gradient est CONSTANT et connu.
static bool GrapheRelief(float32 force, NkString &out) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId sortie = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId bump = NkMatAddNode(g, NK_MN_BUMP);
	const NkNodeId sep = NkMatAddNode(g, NK_MN_SEPARATE_XYZ);
	const NkNodeId coord = NkMatAddNode(g, NK_MN_TEX_COORD);
	g.Connect(bsdf, "bsdf", sortie, "surface");
	g.Connect(bump, "normal", bsdf, "normal");
	g.Connect(sep, "x", bump, "height");
	g.Connect(coord, "uv", sep, "vector");
	g.SetSocketDefault(bump, "strength", NkSocketDir::Input, NkValueReal(t.real, force));
	g.SetSocketDefault(bump, "distance", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	const float32 gris[3] = {0.5f, 0.5f, 0.5f};
	g.SetSocketDefault(bsdf, "base_color", NkSocketDir::Input, NkValueVec(t.color, gris, 3));
	g.SetSocketDefault(bsdf, "metallic", NkSocketDir::Input, NkValueReal(t.real, 0.f));
	g.SetSocketDefault(bsdf, "roughness", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	out = r.source;
	return r.ok;
}

// Une carte de normales PLATE, donnee par un `RGB` a (0,5 ; 0,5 ; 1) — le texel
// neutre. Elle doit rendre exactement la normale geometrique.
static bool GrapheCarteNormales(float32 tr, float32 tv, float32 tb, NkString &out) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId sortie = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId nm = NkMatAddNode(g, NK_MN_NORMAL_MAP);
	const NkNodeId rgb = NkMatAddNode(g, NK_MN_RGB);
	g.Connect(bsdf, "bsdf", sortie, "surface");
	g.Connect(nm, "normal", bsdf, "normal");
	g.Connect(rgb, "color", nm, "color");
	const float32 texel[3] = {tr, tv, tb};
	g.SetProp(rgb, NK_MPROP_COLOR, NkValueVec(t.color, texel, 3));
	const float32 gris[3] = {0.5f, 0.5f, 0.5f};
	g.SetSocketDefault(bsdf, "base_color", NkSocketDir::Input, NkValueVec(t.color, gris, 3));
	g.SetSocketDefault(bsdf, "metallic", NkSocketDir::Input, NkValueReal(t.real, 0.f));
	g.SetSocketDefault(bsdf, "roughness", NkSocketDir::Input, NkValueReal(t.real, 1.f));
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

	// ── 6. Mix Color : l'OPERATION choisie change le pixel, d'un facteur
	//    exactement calculable ──────────────────────────────────────────
	uint8 melange[4] = {}, eclairci[4] = {};
	ok = GrapheMelangeCouleur("melanger", 0.5f, src) && ctx.RendreEtLire(src, "matgraph_mixmelange", melange);
	{
		// « melanger » a fac = 0,5 : mix((E,0,0), (0,E,0), 0,5) = (E/2, E/2, 0).
		// Les deux ecarts valent donc la MOITIE de l'ecart de reference.
		const int32 dR = (int32)melange[0] - (int32)melange[2];
		const int32 dV = (int32)melange[1] - (int32)melange[2];
		Cas("rendu/mixcolor-melanger",
			ok && melange[0] == melange[1] && dR == kEcartAttendu / 2 && dV == kEcartAttendu / 2,
			NkFormat("pixel=({0},{1},{2}) | R==V : {3} | ecarts R-B={4} V-B={5} (attendu {6})", melange[0],
					 melange[1], melange[2], NkString(melange[0] == melange[1] ? "oui" : "NON"), dR, dV,
					 kEcartAttendu / 2));
	}

	ok = GrapheMelangeCouleur("eclaircir", 1.f, src) && ctx.RendreEtLire(src, "matgraph_mixeclair", eclairci);
	{
		// ⚠️ LE CAS QUI PROUVE QUE LA PROPRIETE PILOTE VRAIMENT LE CALCUL.
		// « eclaircir » a fac = 1 : max((E,0,0), (0,E,0)) = (E, E, 0). Les deux
		// ecarts valent donc l'ecart PLEIN — exactement le DOUBLE du cas
		// precedent. Un compilateur qui ignorerait l'operation et melangerait
		// toujours rendrait 64 ici comme la, et la difference entre les deux
		// cas est la seule chose qui le denonce.
		const int32 dR = (int32)eclairci[0] - (int32)eclairci[2];
		const int32 dV = (int32)eclairci[1] - (int32)eclairci[2];
		const int32 dRefR = (int32)melange[0] - (int32)melange[2];
		Cas("rendu/mixcolor-eclaircir-double-melanger",
			ok && eclairci[0] == eclairci[1] && dR == kEcartAttendu && dV == kEcartAttendu && dR == 2 * dRefR,
			NkFormat("pixel=({0},{1},{2}) | R==V : {3} | ecarts={4}/{5} (attendu {6}) | vaut le DOUBLE de "
					 "melanger : {7}",
					 eclairci[0], eclairci[1], eclairci[2], NkString(eclairci[0] == eclairci[1] ? "oui" : "NON"),
					 dR, dV, kEcartAttendu, NkString(dR == 2 * dRefR ? "oui" : "NON")));
	}

	// ── 7. ColorRamp : la charge VARIABLE atteint le GPU ─────────────────
	{
		// Deux arrets : rouge a 0, vert a 1. Trois lectures.
		const float32 deux[8] = {0.f, kE, 0.f, 0.f, 1.f, 0.f, kE, 0.f};
		uint8 a0[4] = {}, a1[4] = {}, am[4] = {};
		const bool c0 = GrapheRampe(deux, 8, 0.f, src) && ctx.RendreEtLire(src, "matgraph_ramp0", a0);
		const bool c1 = GrapheRampe(deux, 8, 1.f, src) && ctx.RendreEtLire(src, "matgraph_ramp1", a1);
		const bool cm = GrapheRampe(deux, 8, 0.5f, src) && ctx.RendreEtLire(src, "matgraph_rampm", am);
		const bool bornes = (a0[0] - a0[2] == kEcartAttendu) && a0[1] == a0[2] &&
							(a1[1] - a1[2] == kEcartAttendu) && a1[0] == a1[2];
		const bool milieu = am[0] == am[1] && (am[0] - am[2]) == kEcartAttendu / 2;
		Cas("rendu/rampe-deux-arrets", c0 && c1 && cm && bornes && milieu,
			NkFormat("fac=0 ({0},{1},{2}) | fac=1 ({3},{4},{5}) | fac=0,5 ({6},{7},{8}) | bornes={9} milieu={10}",
					 a0[0], a0[1], a0[2], a1[0], a1[1], a1[2], am[0], am[1], am[2], bornes ? 1 : 0,
					 milieu ? 1 : 0));
	}
	{
		// ⚠️ LE CAS QUI PROUVE LA CHARGE VARIABLE, ET LUI SEUL.
		// Trois arrets : rouge a 0, VERT AU MILIEU, rouge a 1. A fac=0,5 la
		// couleur doit valoir EXACTEMENT l'arret du milieu — vert plein.
		//
		// Un emetteur qui ne lirait que le PREMIER et le DERNIER arret — l'erreur
		// naturelle quand on traite une liste comme une paire — rendrait du rouge
		// interpole avec du rouge, donc (E, 0, 0) : R==128 et V==B. Les deux
		// resultats sont des couleurs parfaitement plausibles ; seul le canal ou
		// tombe l'ecart les distingue.
		const float32 trois[12] = {0.f, kE, 0.f, 0.f, 0.5f, 0.f, kE, 0.f, 1.f, kE, 0.f, 0.f};
		uint8 px[4] = {};
		const bool c = GrapheRampe(trois, 12, 0.5f, src) && ctx.RendreEtLire(src, "matgraph_ramp3", px);
		const int32 dV = (int32)px[1] - (int32)px[2];
		const bool arretDuMilieuHonore = px[0] == px[2] && dV == kEcartAttendu;
		Cas("rendu/rampe-arret-du-milieu-honore", c && arretDuMilieuHonore,
			NkFormat("pixel=({0},{1},{2}) | R==B : {3} | ecart V-B={4} (attendu {5} : l'arret du milieu, pas une "
					 "interpolation des bords)",
					 px[0], px[1], px[2], NkString(px[0] == px[2] ? "oui" : "NON"), dV, kEcartAttendu));
	}

	// ── 8. le relief : la carte plate, puis le SENS de la bosse ─────────
	uint8 plate[4] = {}, geo[4] = {};
	{
		// ⚠️ LE PREMIER DES DEUX CONTROLES : une carte plate (0,5 ; 0,5 ; 1) doit
		// rendre EXACTEMENT la normale geometrique. On compare donc au meme
		// graphe dont la normale est imposee a (0, 0, 1) — trois canaux, au bit.
		//
		// Ce cas passe dans LES DEUX conventions : c'est justement pourquoi il ne
		// suffit pas, et pourquoi le suivant existe.
		const bool a = GrapheCarteNormales(0.5f, 0.5f, 1.f, src) && ctx.RendreEtLire(src, "matgraph_platte", plate);
		const bool b = GrapheNormaleImposee(0.f, 0.f, 1.f, src) && ctx.RendreEtLire(src, "matgraph_geo", geo);
		const bool egal = plate[0] == geo[0] && plate[1] == geo[1] && plate[2] == geo[2];
		Cas("rendu/carte-plate-rend-la-normale-geometrique", a && b && egal,
			NkFormat("carte plate ({0},{1},{2}) | normale geometrique imposee ({3},{4},{5}) | identiques={6}",
					 plate[0], plate[1], plate[2], geo[0], geo[1], geo[2], egal ? 1 : 0));
	}
	{
		// ⚠️ LE CONTROLE QUI ATTRAPE L'INVERSION, et il ne reimplante rien.
		//
		// On ETALONNE d'abord : le meme shader, le meme eclairage, mais une
		// normale imposee inclinee vers -x puis vers +x. Ces deux rendus donnent
		// les deux couleurs possibles SANS que le banc calcule quoi que ce soit.
		// Puis on rend le relief a force +1 et -1.
		//
		// Ce qui est exige : les deux reliefs DIFFERENT (sinon le signe est
		// ignore), et chacun tombe sur l'UN des deux etalons — donc le relief
		// incline la surface du bon cote, et l'inverser inverse l'eclairage.
		uint8 versMoinsX[4] = {}, versPlusX[4] = {}, bPlus[4] = {}, bMoins[4] = {};
		const bool e1 = GrapheNormaleImposee(-0.5f, 0.f, 1.f, src) &&
						ctx.RendreEtLire(src, "matgraph_etal_mx", versMoinsX);
		const bool e2 = GrapheNormaleImposee(0.5f, 0.f, 1.f, src) &&
						ctx.RendreEtLire(src, "matgraph_etal_px", versPlusX);
		const bool r1 = GrapheRelief(1.f, src) && ctx.RendreEtLire(src, "matgraph_bump_p", bPlus);
		const bool r2 = GrapheRelief(-1.f, src) && ctx.RendreEtLire(src, "matgraph_bump_m", bMoins);

		auto memePixel = [](const uint8 *a, const uint8 *b) {
			return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
		};
		const bool etalonsDifferent = !memePixel(versMoinsX, versPlusX);
		const bool reliefsDifferent = !memePixel(bPlus, bMoins);
		// ⚠️ L'APPARIEMENT EST ORIENTE, et il DOIT l'etre. Ma premiere version
		// acceptait « chacun tombe sur l'un des deux etalons », dans n'importe
		// quel ordre — une INVERSION GLOBALE du signe du relief l'aurait donc
		// passee au vert, puisque les deux resultats se contentent d'echanger.
		// C'est exactement le defaut que ce cas existe pour attraper.
		//
		// Le sens se DEDUIT DU GRAPHE, sans reimplanter le moindre ombrage : la
		// hauteur vaut `u`, qui croit avec +x ; son gradient pointe donc vers +x ;
		// et la formule du relief incline la normale a L'OPPOSE du gradient
		// (N = geomN - force x distance x gradient). Une force POSITIVE doit donc
		// rendre l'etalon incline vers **-x**, et une force negative l'autre.
		const bool appariement = memePixel(bPlus, versMoinsX) && memePixel(bMoins, versPlusX);
		Cas("rendu/relief-incline-du-bon-cote",
			e1 && e2 && r1 && r2 && etalonsDifferent && reliefsDifferent && appariement,
			NkFormat("etalons -x({0},{1},{2}) +x({3},{4},{5}) differents={6} | relief +1({7},{8},{9}) "
					 "-1({10},{11},{12}) differents={13} | force +1 tombe sur l etalon -x et -1 sur +x={14}",
					 versMoinsX[0], versMoinsX[1], versMoinsX[2], versPlusX[0], versPlusX[1], versPlusX[2],
					 etalonsDifferent ? 1 : 0, bPlus[0], bPlus[1], bPlus[2], bMoins[0], bMoins[1], bMoins[2],
					 reliefsDifferent ? 1 : 0, appariement ? 1 : 0));
	}

	{
		// ⚠️ CE CAS EXISTE PARCE QU'UNE MUTATION A SURVECU. La carte PLATE ne peut
		// pas voir une base tangente cassee : avec un texel (0,5 ; 0,5 ; 1) la
		// normale tangente vaut (0, 0, 1), donc T et B sont multiplies par ZERO et
		// n'interviennent pas. Le cas precedent est vrai et necessaire — il ne
		// suffit pas.
		//
		// On rend donc une carte INCLINEE : texel (0,75 ; 0,5 ; 1) donne une
		// normale tangente (0,5 ; 0 ; 1), qui penche vers +u — c'est-a-dire vers
		// +x, puisque l'UV croit avec x. Elle doit tomber du cote de l'etalon +x,
		// donc etre PLUS SOMBRE que la carte plate.
		uint8 penchee[4] = {}, plateBis[4] = {}, etalonPlusX[4] = {};
		const bool a = GrapheCarteNormales(0.75f, 0.5f, 1.f, src) &&
					   ctx.RendreEtLire(src, "matgraph_nm_penchee", penchee);
		const bool b = GrapheCarteNormales(0.5f, 0.5f, 1.f, src) &&
					   ctx.RendreEtLire(src, "matgraph_nm_plate", plateBis);
		const bool c = GrapheNormaleImposee(0.5f, 0.f, 1.f, src) &&
					   ctx.RendreEtLire(src, "matgraph_nm_etal", etalonPlusX);
		// La base tangente agit : la carte penchee differe de la plate.
		const bool agit = a && b && (penchee[0] != plateBis[0]);
		// Et elle penche du BON cote : vers +x, donc du cote sombre.
		const bool bonCote = a && b && c && penchee[0] < plateBis[0] && penchee[0] >= etalonPlusX[0];
		Cas("rendu/carte-penchee-suit-la-base-tangente", agit && bonCote,
			NkFormat("penchee({0}) | plate({1}) | etalon +x({2}) | la base tangente agit={3} | penche du cote +x, "
					 "donc plus sombre que plate et pas au-dela de l etalon={4}",
					 penchee[0], plateBis[0], etalonPlusX[0], agit ? 1 : 0, bonCote ? 1 : 0));
	}

	// ── 9. le fond de repli n'a jamais ete lu ────────────────────────────
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
