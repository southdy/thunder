#ifndef POSTPROCESSVOLUME_H
#define POSTPROCESSVOLUME_H

#include "component.h"

class PostProcessVolumePrivate;

class PostProcessSettings;

class NEXT_LIBRARY_EXPORT PostProcessVolume : public Component {
    A_PROPERTIES(
        A_PROPERTY(int, priority, PostProcessVolume::priority, PostProcessVolume::setPriority),
        A_PROPERTY(bool, unbound, PostProcessVolume::unbound, PostProcessVolume::setUnbound)
    )
    A_NOMETHODS()

public:
    PostProcessVolume();
    ~PostProcessVolume();

    int priority() const;
    void setPriority(int priority);

    bool unbound() const;
    void setUnbound(bool unbound);

    float blendWeight() const;
    void setBlendWeight(float weight);

    AABBox bound() const;

    const PostProcessSettings &settings() const;

    static const MetaObject *metaClass();

public:
    static void registerClassFactory(ObjectSystem *system);
    static void unregisterClassFactory(ObjectSystem *system);

private:
    static Object *construct();

    const MetaObject *metaObject() const override;

    Variant readProperty(const MetaProperty &property) const;

    void writeProperty(const MetaProperty &property, const Variant &value);

    bool isPostProcessVolume() const override;

#ifdef NEXT_SHARED
    bool drawHandles(ObjectList &selected) override;
#endif

private:
    PostProcessVolumePrivate *p_ptr;

};

#endif // POSTPROCESSVOLUME_H
