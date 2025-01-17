#include "prefabconverter.h"

#include "components/actor.h"

#include <QFile>
#include <QDebug>

#include <bson.h>
#include <json.h>

#define FORMAT_VERSION 2

PrefabConverterSettings::PrefabConverterSettings() {
    setType(MetaType::type<Prefab *>());
    setVersion(FORMAT_VERSION);
}

IConverterSettings *PrefabConverter::createSettings() const {
    return new PrefabConverterSettings();
}

Actor *PrefabConverter::createActor(const QString &guid) const {
    PROFILE_FUNCTION();

    Prefab *prefab = Engine::loadResource<Prefab>(guid.toStdString());
    if(prefab) {
        return static_cast<Actor *>(prefab->actor()->clone());
    }
    return IConverter::createActor(guid);
}

uint8_t PrefabConverter::convertFile(IConverterSettings *settings) {
    PROFILE_FUNCTION();

    QFile src(settings->source());
    if(src.open(QIODevice::ReadOnly)) {
        string data = src.readAll().toStdString();
        src.close();

        Variant actor = readJson(data, settings);
        injectResource(actor, Engine::objectCreate<Prefab>(""));

        QFile file(settings->absoluteDestination());
        if(file.open(QIODevice::WriteOnly)) {
            ByteArray data = Bson::save(actor);
            file.write((const char *)&data[0], data.size());
            file.close();

            return 0;
        }
    }
    return 1;
}

Variant PrefabConverter::readJson(const string &data, IConverterSettings *settings) {
    PROFILE_FUNCTION();

    Variant result = Json::load(data);

    bool update = false;
    switch(settings->currentVersion()) {
        case 0: toVersion1(result); update = true;
        default: break;
    }

    if(update) {
        QFile src(settings->source());
        if(src.open(QIODevice::WriteOnly)) {
            settings->setCurrentVersion(settings->version());

            string data = Json::save(result, 0);
            src.write(data.c_str(), data.size());
            src.close();
        }
    }

    return result;
}

void PrefabConverter::injectResource(Variant &origin, Resource *resource) {
    PROFILE_FUNCTION();

    VariantList &objects = *(reinterpret_cast<VariantList *>(origin.data()));
    VariantList &o = *(reinterpret_cast<VariantList *>(objects.front().data()));

    auto i = o.begin();
    string type = i->toString();
    i++; // uuid
    i++; // parent
    *i = resource->uuid();

    objects.push_front(Engine::toVariant(resource).toList().front());
}

void PrefabConverter::toVersion1(Variant &variant) {
    PROFILE_FUNCTION();

    // Create all declared objects
    VariantList &objects = *(reinterpret_cast<VariantList *>(variant.data()));
    for(auto &it : objects) {
        VariantList &o = *(reinterpret_cast<VariantList *>(it.data()));
        if(o.size() >= 5) {
            auto i = o.begin();
            ++i;
            ++i;
            ++i;
            ++i;

            // Load base properties
            VariantMap &properties = *(reinterpret_cast<VariantMap *>((*i).data()));
            VariantMap propertiesNew;
            for(auto &prop : properties) {
                QString property(prop.first.c_str());

                property.replace("_Rotation", "quaternion");
                property.replace("Use_Kerning", "kerning");
                property.replace("Audio_Clip", "clip");
                property.replace('_', "");
                property.replace(0, 1, property[0].toLower());

                propertiesNew[property.toStdString()] = prop.second;
            }
            properties = propertiesNew;
        }
    }
}
