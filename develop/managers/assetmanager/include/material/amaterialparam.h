#ifndef MATERIALPARAM
#define MATERIALPARAM

#include "../shaderbuilder.h"

class ParamFloat : public ShaderFunction {
    Q_OBJECT
    Q_CLASSINFO("Group", "Parameters")

    Q_PROPERTY(QString Parameter_Name READ objectName WRITE setObjectName DESIGNABLE true USER true)

public:
    Q_INVOKABLE ParamFloat() {
        connect(this, SIGNAL(objectNameChanged(QString)), this, SIGNAL(updated()));
    }

    virtual AbstractSchemeModel::Node  *createNode  (ShaderBuilder *model, const QString &path) {
        AbstractSchemeModel::Node *result   = ShaderFunction::createNode(model, path);
        AbstractSchemeModel::Item *out      = new AbstractSchemeModel::Item;
        out->name   = "";
        out->out    = true;
        out->pos    = 0;
        out->type   = QMetaType::Double;
        result->list.push_back(out);

        return result;
    }

    bool build(QString &value, const AbstractSchemeModel::Link &, uint32_t &depth, uint8_t &size) {
        size    = MetaType::FLOAT;
        m_pModel->addUniform(objectName(), size);
        value  += QString("\tfloat local%1 = %2;\n").arg(depth).arg(objectName());

        return true;
    }
};

class ParamVector : public ShaderFunction {
    Q_OBJECT
    Q_CLASSINFO("Group", "Parameters")

    Q_PROPERTY(QString Parameter_Name READ objectName WRITE setObjectName DESIGNABLE true USER true)

public:
    Q_INVOKABLE ParamVector() {
        connect(this, SIGNAL(objectNameChanged(QString)), this, SIGNAL(updated()));
    }

    virtual AbstractSchemeModel::Node  *createNode  (ShaderBuilder *model, const QString &path) {
        AbstractSchemeModel::Node *result   = ShaderFunction::createNode(model, path);
        AbstractSchemeModel::Item *out      = new AbstractSchemeModel::Item;
        out->name   = "";
        out->out    = true;
        out->pos    = 0;
        out->type   = QMetaType::QVector4D;
        result->list.push_back(out);

        return result;
    }

    bool build(QString &value, const AbstractSchemeModel::Link &, uint32_t &depth, uint8_t &size) {
        size    = QMetaType::QVector4D;
        m_pModel->addUniform(objectName(), size);
        value  += QString("\tvec4 local%1 = %2;\n").arg(depth).arg(objectName());
        return true;
    }
};

#endif // MATERIALPARAM

