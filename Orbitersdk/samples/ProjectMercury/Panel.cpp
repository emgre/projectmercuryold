#include "Panel.h"
#include "MercuryCapsule.h"
#include "PanelElement.h"
#include "PanelTexture.h"
#include "PanelMesh.h"
#include <vector>

Panel::Panel(MercuryCapsule* vessel)
:VesselComponent(vessel)
{
	hMesh = NULL;
	pMeshBackground = NULL;
	scrollFlags = 0;
}

Panel::~Panel()
{
	if (hMesh)
	{
		oapiDeleteMesh(hMesh);
	}
}

void Panel::LoadPanel(PANELHANDLE hPanel, DWORD viewW, DWORD viewH)
{
	if (!hMesh)
	{
		CreateMesh();
	}

	for (PanelElement* elem : pPanelElements)
	{
		elem->Reset();
	}

	SURFHANDLE* surf = new SURFHANDLE[pTextures.size()];
	for (unsigned int i = 0; i < pTextures.size(); i++)
	{
		surf[i] = pTextures[i]->GetSurfhandle();
	}

	pVessel->SetPanelBackground(hPanel, surf, pTextures.size(), hMesh, (DWORD)pMeshBackground->GetWidth(), (DWORD)pMeshBackground->GetHeight(), 0, scrollFlags);

	delete[] surf;
}

void Panel::SetBackgroundMesh(PanelMesh* mesh)
{
	if (mesh)
	{
		pMeshBackground = mesh;
	}
}

void Panel::SetScrollFlags(DWORD flags)
{
	scrollFlags = flags;
}

void Panel::AddPanelElement(PanelElement* elem)
{
	if (elem)
	{
		pPanelElements.push_back(elem);
	}
}

void Panel::CreateMesh()
{
	pTextures.clear();

	// Create the mesh.
	if (hMesh)
	{
		oapiDeleteMesh(hMesh);
	}
	hMesh = oapiCreateMesh(0, NULL);

	// Add background.
	std::vector<PanelMesh*> meshes;
	if (pMeshBackground)
	{
		meshes.push_back(pMeshBackground);
		GenerateMeshGroups(meshes);
	}
	else
	{
		sprintf(oapiDebugString(), "WARNING: There is a missing background on a panel.");
	}

	meshes.clear();

	// Add elements.
	for (PanelElement* elem : pPanelElements)
	{
		auto newMeshes = elem->GetPanelMesh();
		meshes.insert(meshes.end(), newMeshes.begin(), newMeshes.end());
	}
	GenerateMeshGroups(meshes);
}

void Panel::GenerateMeshGroups(const std::vector<PanelMesh*> &meshes)
{
	std::map<int, MESHGROUP> groups;

	// Separate meshes by texture index.
	for (PanelMesh* mesh : meshes)
	{
		// Add missing textures.
		int textureId;
		PanelTexture* tex = mesh->GetTexture();
		auto it = std::find(pTextures.begin(), pTextures.end(), tex);
		if (it == pTextures.end())
		{
			pTextures.push_back(tex);
			textureId = pTextures.size() - 1;
		}
		else
		{
			textureId = it - pTextures.begin();
		}

		// Create new mesh group.
		if (groups.count(textureId) == 0)
		{
			MESHGROUP newMeshGroup;
			memset(&newMeshGroup, 0, sizeof(MESHGROUP));
			newMeshGroup.TexIdx = textureId;
			newMeshGroup.nVtx = 4;
			newMeshGroup.nIdx = 6;
			groups[textureId] = newMeshGroup;
		}

		// Add number of vertices and indices to mesh group.
		else
		{
			groups[textureId].nVtx += 4;
			groups[textureId].nIdx += 6;
		}
	}

	// Send mesh groups to Orbiter.
	std::map<int, MESHGROUP*> result;
	for (std::pair<int, MESHGROUP> grp : groups)
	{
		int id = oapiAddMeshGroup(hMesh, &grp.second);
		result[grp.first] = oapiMeshGroup(hMesh, id);
	}

	// Add data to the mesh groups.
	std::vector<int> offsets;
	for (PanelTexture* tex : pTextures)
	{
		offsets.push_back(0);
	}

	for (PanelMesh* mesh : meshes)
	{
		PanelTexture* tex = mesh->GetTexture();

		auto it = std::find(pTextures.begin(), pTextures.end(), tex);
		int id = it - pTextures.begin();
		int offset = offsets[id];

		MESHGROUP* grp = result[id];
		mesh->SetMeshData(&grp->Vtx[4 * offset]);

		grp->Idx[6 * offset + 0] = 4 * offset + 0;
		grp->Idx[6 * offset + 1] = 4 * offset + 1;
		grp->Idx[6 * offset + 2] = 4 * offset + 2;
		grp->Idx[6 * offset + 3] = 4 * offset + 2;
		grp->Idx[6 * offset + 4] = 4 * offset + 3;
		grp->Idx[6 * offset + 5] = 4 * offset + 0;

		offsets[id]++;
	}
}