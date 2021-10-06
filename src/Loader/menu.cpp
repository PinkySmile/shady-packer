#include "main.hpp"
#include "modpackage.hpp"
#include "menu.hpp"

namespace {
	ModPackage* selectedPackage = 0;
	std::list<ModPackage*> downloads;

	enum :int { OPTION_ENABLE_DISABLE, OPTION_DOWNLOAD };
}

static void setPackageEnabled(ModPackage* package, bool value) {
	if (package->enabled == value) return;
	package->enabled = value;
	if (package->enabled) {
		EnablePackage(package);
	} else {
		DisablePackage(package);
	}
}

ModList::ModList() : CFileList() {
	this->maxLength = 26;
	this->extLength = 0;
}

void ModList::renderScroll(float x, float y, int offset, int size, int view) {
	// just set values, render is done on CDesign
	this->scrollLen = size < view ? 286 : view*286/size;
	this->scrollBar->y1 = 286*offset/size + this->scrollLen - 286;

	for (int i = 0; i < view && i < size - offset; ++i) {
		this->renderLine(x, y + i*16, i + offset);
	}
}

void ModList::updateList() {
	this->names.clear();
	this->types.clear();
	ModPackage::packageListMutex.lock_shared();
	for (auto package : ModPackage::packageList) {
		auto name = package->name.string();
		SokuLib::String str; str.assign(name.c_str(), name.length());
		this->names.push_back(str);
		this->types.push_back(package->enabled);
	} ModPackage::packageListMutex.unlock_shared();
	this->updateResources();
}

int ModList::appendLine(SokuLib::String& out, void* unknown, SokuLib::Deque<SokuLib::String>& list, int index) {
	std::shared_lock lock(ModPackage::packageListMutex);
	ModPackage* package = ModPackage::packageList[index];
	int color = package->requireUpdate ? 0xff8040
		: package->enabled ? 0x40ff40
		: package->isLocal() ? 0x6060d0
		: 0x808080;

	char buffer[15]; sprintf(buffer, "<color %06x>", color);
	out.append(buffer, 14);
	int len = CFileList::appendLine(out, unknown, list, index);
	out.append("</color>", 8);

	return len;
}

ModMenu::ModMenu() {
	design.loadResource("shady/downloader.dat");
	modList.updateList();

	design.getById((SokuLib::CDesign::Sprite**)&modList.scrollBar, 101);
	modList.scrollBar->active = true;
	modList.scrollBar->gauge.set(&modList.scrollLen, 0, 286);
	modCursor.set(&SokuLib::inputMgrs.input.verticalAxis, modList.names.size());
	this->updateView(modCursor.pos);

	ModPackage::LoadFromRemote();
}

ModMenu::~ModMenu() {
	design.clear();
	modList.clear();
	if (viewTitle.texture) SokuLib::textureMgr.remove(viewTitle.texture);
	if (viewContent.texture) SokuLib::textureMgr.remove(viewContent.texture);
	if (viewOption.texture) SokuLib::textureMgr.remove(viewOption.texture);
	if (viewPreview.texture) SokuLib::textureMgr.remove(viewPreview.texture);
}

void ModMenu::_() {}

int ModMenu::onProcess() {
	if (ModPackage::Notify()) {
		modList.updateList();
		modCursor.set(&SokuLib::inputMgrs.input.verticalAxis, modList.names.size(), modCursor.pos);
		this->updateView(modCursor.pos);
	}

	// Cursor On List
	if (this->state == 0) {
		if (modCursor.update()) {
			SokuLib::playSEWaveBuffer(0x27);
			this->updateView(modCursor.pos);
		}

		if (SokuLib::inputMgrs.input.b == 1) {
			SokuLib::playSEWaveBuffer(0x29);
			return false; // close
		}

		if (SokuLib::inputMgrs.input.a == 1 && this->optionCount) {
			SokuLib::playSEWaveBuffer(0x28);
			this->state = 1;
			viewCursor.set(&SokuLib::inputMgrs.input.verticalAxis, this->optionCount);
		}
	// Cursor On View
	} else if (this->state == 1) {
		if (viewCursor.update()) {
			SokuLib::playSEWaveBuffer(0x27);
		}

		if (SokuLib::inputMgrs.input.b == 1) {
			SokuLib::playSEWaveBuffer(0x29);
			this->state = 0;
			return true;
		}

		if (SokuLib::inputMgrs.input.a == 1) {
			std::shared_lock lock(ModPackage::packageListMutex);
			SokuLib::playSEWaveBuffer(0x28);
			ModPackage* package = ModPackage::packageList[modCursor.pos];
			switch (options[viewCursor.pos]) {
			case OPTION_ENABLE_DISABLE:
				setPackageEnabled(package, !package->enabled);
				SaveSettings();
				modList.updateList();
				break;
			case OPTION_DOWNLOAD:
				package->downloadFile();
				this->state = 0;
				break;
			}

			this->updateView(modCursor.pos);
		}
	}

	return !SokuLib::checkKeyOneshot(0x1, false, false, false);
}

int ModMenu::onRender() {
	design.render4();

	if (modCursor.pos > scrollPos + 16) {
		scrollPos = modCursor.pos - 16;
	} else if (modCursor.pos < scrollPos) {
		scrollPos = modCursor.pos;
	}

	SokuLib::CDesign::Object* pos;
	design.getById(&pos, 100);
	if (this->state == 0) SokuLib::MenuCursor::render(pos->x2, pos->y2 + (modCursor.pos - scrollPos)*16, 256);
	modList.renderScroll(pos->x2, pos->y2, scrollPos, modList.getLength(), 17);

	design.getById(&pos, 200);
	viewTitle.render(pos->x2, pos->y2);
	design.getById(&pos, 201);
	viewContent.render(pos->x2, pos->y2);
	design.getById(&pos, 202);
	if (this->state == 1) SokuLib::MenuCursor::render(pos->x2, pos->y2 + viewCursor.pos*16, 120);
	viewOption.render(pos->x2, pos->y2);
	design.getById(&pos, 203);
	if(viewPreview.texture) viewPreview.renderScreen(pos->x2, pos->y2, pos->x2 + 200, pos->y2 + 150);

	return 0;
}

void ModMenu::updateView(int index) {
	std::shared_lock lock(ModPackage::packageListMutex);
	ModPackage* package = ModPackage::packageList[index];
	SokuLib::FontDescription fontDesc {
		"",
		0xff, 0xa0, 0xff, 0xa0, 0xff, 0xff,
		20, 400,
		false, true, false,
		100000, 0, 0, 0, 2
	}; strcpy(fontDesc.faceName, SokuLib::defaultFontName);
	SokuLib::SWRFont font; font.create();
	int textureId;

	font.setIndirect(fontDesc);
	SokuLib::textureMgr.createTextTexture(&textureId, package->name.string().c_str(), font, 220, 24, 0, 0);
	if (viewTitle.texture) SokuLib::textureMgr.remove(viewTitle.texture);
	viewTitle.setTexture2(textureId, 0, 0, 220, 24);

	fontDesc.weight = 300;
	fontDesc.height = 14;
	fontDesc.useOffset = true;
	font.setIndirect(fontDesc);
	std::string temp;
	if (package->isLocal()) temp = "<color 404040>This is a local Package.</color>";
	else {
		temp += "Version: <color 404040>" + package->version() + "</color><br>";
		temp += "Creator: <color 404040>" + package->creator() + "</color><br>";
		temp += "Description: <color 404040>" + package->description() + "</color><br>";
		temp += "Tags: ";
		for (int i = 0; i < package->tags.size(); ++i) {
			if (i > 0) temp += ", ";
			temp += "<color 404040>" + package->tags[i] + "</color>";
		}
	}
	// TODO status
	SokuLib::textureMgr.createTextTexture(&textureId, temp.c_str(), font, 220, 190, 0, 0);
	if (viewContent.texture) SokuLib::textureMgr.remove(viewContent.texture);
	viewContent.setTexture2(textureId, 0, 0, 220, 190);

	if (package->downloading) {
		this->optionCount = 0;
		temp = "Downloading ...";
	} else if (package->fileExists) {
		temp = (package->enabled ? "Disable<br>" : "Enable<br>");
		this->options[0] = OPTION_ENABLE_DISABLE;
		if (package->requireUpdate) {
			temp += "Update";
			this->options[1] = OPTION_DOWNLOAD;
			this->optionCount = 2;
		} else {
			this->optionCount = 1;
		}
	} else {
		temp = "Download";
		this->options[0] = OPTION_DOWNLOAD;
		this->optionCount = 1;
	}
	SokuLib::textureMgr.createTextTexture(&textureId, temp.c_str(), font, 220, 40, 0, 0);
	if (viewOption.texture) SokuLib::textureMgr.remove(viewOption.texture);
	viewOption.setTexture2(textureId, 0, 0, 220, 40);

	if (viewPreview.texture) SokuLib::textureMgr.remove(viewPreview.texture);
	if (!package->previewPath.empty()) {
		int width, height;
		SokuLib::textureMgr.loadTexture(&textureId, package->previewPath.c_str(), &width, &height);
		viewPreview.setTexture2(textureId, 0, 0, width, height);
	} else {
		package->downloadPreview();
		viewPreview.texture = 0;
	}
}

void LoadPackage() {
	ModPackage::LoadFromLocalData();
	ModPackage::LoadFromFilesystem();

	std::shared_lock lock(ModPackage::packageListMutex);
	for (auto& package : ModPackage::packageList) {
		bool isEnabled = GetPrivateProfileIntW(L"Packages", package->name.c_str(),
			false, (ModPackage::basePath / L"shady-loader.ini").c_str());
		setPackageEnabled(package, isEnabled);
	}
}

void UnloadPackage() {
	std::shared_lock lock(ModPackage::packageListMutex);
	loadLock.lock();
	for (auto& package : ModPackage::packageList) {
		setPackageEnabled(package, false);
		delete package;
	}
	loadLock.unlock();
	ModPackage::packageList.clear();
}
