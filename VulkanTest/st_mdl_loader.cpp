#include "st_mdl_loader.h"
#include "st_utils.h"
#include <unordered_map>
#include "st_material_management.h"




namespace vtx {

	struct FileHeader_t {
		// file version as defined by VHV_VERSION
		int version;

		// hardware params that affect how the model is to be optimized.
		int vertCacheSize;
		unsigned short maxBonesPerStrip;
		unsigned short maxBonesPerFace;
		int maxBonesPerVert;

		// must match checkSum in the .mdl
		int checkSum;

		int numLODs; // garymcthack - this is also specified in ModelHeader_t and should match

		// one of these for each LOD
		int materialReplacementListOffset;

		int numBodyParts;
		int bodyPartOffset;
	};

	struct BodyPartHeader_t {
		int numModels;
		int modelOffset;
	};

	struct ModelHeader_t {
		int numLODs;
		int lodOffset;
	};

	struct ModelLODHeader_t
	{
		int numMeshes;
		int meshOffset;
		float switchPoint;
	};
#pragma pack(push,1)
	struct MeshHeader_t
	{

		int numStripGroups;
		int stripGroupHeaderOffset;

		unsigned char flags;
	};
#pragma pack(pop)
	struct StripGroupHeader_t {
		int numVerts;
		int vertOffset;
		int numIndices;
		int indexOffset;
		int numStrips;
		int stripOffset;
		unsigned char flags;
	};

	struct StripHeader_t
	{

		// indexOffset offsets into the mesh's index array.
		int numIndices;
		int indexOffset;

		// vertexOffset offsets into the mesh's vert array.
		int numVerts;
		int vertOffset;

		// use this to enable/disable skinning.  
		// May decide (in optimize.cpp) to put all with 1 bone in a different strip 
		// than those that need skinning.
		short numBones;

		unsigned char flags;

		int numBoneStateChanges;
		int boneStateChangeOffset;

	};

#pragma pack(push,1)
	struct  Vertex_t
	{
		// these index into the mesh's vert[origMeshVertID]'s bones
		unsigned char boneWeightIndex[MAX_NUM_BONES_PER_VERT];
		unsigned char numBones;

		unsigned short origMeshVertID;

		// for sw skinned verts, these are indices into the global list of bones
		// for hw skinned verts, these are hardware bone indices
		char boneID[MAX_NUM_BONES_PER_VERT];
	};
#pragma pack(pop)
}


namespace st {





	void MdlLoader::loadFileMultipleMeshes(const char* fileName) {

	}


	void MdlLoader::loadFileSingleMesh(const char* fileName) {
		name = fileName;


		file.open(fileName, std::ios::binary);
		if (!file.good()) {
			printf("missing model %s\n", fileName);
			return;
		}
		file.seekg(0, std::ios::beg);
		file.read((char*)&header, sizeof(header));
		file.seekg(header.vvdOffset, std::ios::beg);
		char* vvdData = (char*)malloc(header.vvdSize);
		file.read(vvdData, header.vvdSize);
		vvd::vertexFileHeader_t* vvdHdr = (vvd::vertexFileHeader_t*)vvdData;
		vvd::mstudiovertex_t* vvdV = (vvd::mstudiovertex_t*)&vvdData[vvdHdr->vertexDataStart];
		std::vector<vvd::mstudiovertex_t> vvdVerts;

		if (vvdHdr->numFixups) {
			vvd::vertexFileFixup_t* fixups = (vvd::vertexFileFixup_t*)&vvdData[vvdHdr->fixupTableStart];
			for (int i = 0; i < vvdHdr->numFixups; i++) {
				//if(fixups[i].lod != 0)continue;
				for (int j = 0; j < fixups[i].numVertexes; j++) {
					vvdVerts.push_back(vvdV[fixups[i].sourceVertexID + j]);
				}
			}
		}
		else {
			for (int i = 0; i < vvdHdr->numLODVertexes[0]; i++) {
				vvdVerts.push_back(vvdV[i]);
			}
		}




		free(vvdData);
		Mesh mesh;



		std::vector<std::string> textureNames;
		for (int i = 0; i < header.numtextures; i++) {
			file.seekg(header.textureindex + sizeof(r2::mstudiotexture_t) * i);
			r2::mstudiotexture_t texture;
			file.read((char*)&texture, sizeof(r2::mstudiotexture_t));
			file.seekg(header.textureindex + sizeof(r2::mstudiotexture_t) * i + texture.sznameindex);
			char readBuffer[128];
			file.read(readBuffer, sizeof(readBuffer));
			textureNames.push_back(readBuffer);
		}



		char* vtxData = (char*)malloc(header.vtxSize);
		file.seekg(header.vtxOffset);
		file.read((char*)vtxData, header.vtxSize);
		vtx::FileHeader_t* vtxHdr = (vtx::FileHeader_t*)vtxData;

		for (int i = 0; i < vtxHdr->numBodyParts; i++) {
			size_t vtxBodyPartOffset = vtxHdr->bodyPartOffset + sizeof(vtx::BodyPartHeader_t) * i;
			vtx::BodyPartHeader_t* bodyPartHdr = (vtx::BodyPartHeader_t*)&vtxData[vtxBodyPartOffset];
			size_t mdlBodyPartOffset = header.bodypartindex + sizeof(r2::mstudiobodyparts_t) * i;
			file.seekg(mdlBodyPartOffset);
			r2::mstudiobodyparts_t mdlBodypart;
			file.read((char*)&mdlBodypart, sizeof(r2::mstudiobodyparts_t));
			for (int j = 0; j < bodyPartHdr->numModels; j++) {
				size_t vtxMdlOffset = vtxBodyPartOffset + bodyPartHdr->modelOffset + sizeof(vtx::ModelHeader_t) * j;
				vtx::ModelHeader_t* vtxMdlHdr = (vtx::ModelHeader_t*)&vtxData[vtxMdlOffset];
				//dont iterate lods only use first(highest quality)
				size_t vtxLodOffset = vtxMdlOffset + vtxMdlHdr->lodOffset;
				vtx::ModelLODHeader_t* lodHdr = (vtx::ModelLODHeader_t*)&vtxData[vtxLodOffset];

				size_t modelOffset = mdlBodyPartOffset + mdlBodypart.modelindex + sizeof(r2::mstudiomodel_t) * j;
				file.seekg(modelOffset);
				r2::mstudiomodel_t mdlModel;
				file.read((char*)&mdlModel, sizeof(r2::mstudiomodel_t));
				for (int k = 0; k < lodHdr->numMeshes; k++) {
					size_t vtxMeshOffset = vtxLodOffset + lodHdr->meshOffset + sizeof(vtx::MeshHeader_t) * k;
					vtx::MeshHeader_t* meshHdr = (vtx::MeshHeader_t*)&vtxData[vtxMeshOffset];

					size_t mdlMeshOffset = modelOffset + mdlModel.meshindex + sizeof(r2::mstudiomesh_t) * k;
					file.seekg(mdlMeshOffset);
					r2::mstudiomesh_t mdlMesh;
					file.read((char*)&mdlMesh, sizeof(r2::mstudiomesh_t));

					for (int l = 0; l < meshHdr->numStripGroups; l++) {
						size_t stripGroupOffset = vtxMeshOffset + meshHdr->stripGroupHeaderOffset + sizeof(vtx::StripGroupHeader_t) * l;
						vtx::StripGroupHeader_t* stripGroupHdr = (vtx::StripGroupHeader_t*)&vtxData[stripGroupOffset];
						vtx::Vertex_t* groupVerts = (vtx::Vertex_t*)&vtxData[stripGroupOffset + stripGroupHdr->vertOffset];
						uint16_t* groupIndices = (uint16_t*)&vtxData[stripGroupOffset + stripGroupHdr->indexOffset];

						for (int m = 0; m < stripGroupHdr->numStrips; m++) {
							size_t stripOffset = stripGroupOffset + stripGroupHdr->stripOffset + sizeof(vtx::StripHeader_t) * m;
							vtx::StripHeader_t* stripHdr = (vtx::StripHeader_t*)&vtxData[stripOffset];
							vtx::Vertex_t* stripVerts = &groupVerts[stripHdr->vertOffset];
							uint16_t* stripIndices = &groupIndices[stripHdr->indexOffset];
							for (int n = 0; n < stripHdr->numIndices; n++) {
								st::Vertex v;
								vtx::Vertex_t vtxVert = stripVerts[stripIndices[n]];
								vvd::mstudiovertex_t& vvdVert = vvdVerts[vtxVert.origMeshVertID];
								v.position = vvdVert.m_vecPosition;
								v.uv = vvdVert.m_vecTexCoord;
								v.materialId = StMaterialManager::getManager().addMaterial(textureNames[mdlMesh.material]);
								std::hash<std::string> strHasher;
								uint32_t materialHash = strHasher(textureNames[mdlMesh.material]);
								v.textureColor = { ((materialHash) & 0xFF) / 255.0,((materialHash >> 8) & 0xFF) / 255.0,((materialHash >> 16) & 0xFF) / 255.0 };
								mesh.addVert(v);
							}
						}

					}
				}
			}
		}


		mesh.finishMesh();
		free(vtxData);
		meshes.push_back(mesh);


	}

}