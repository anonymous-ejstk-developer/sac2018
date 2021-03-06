package ast_node;

import java.util.List;

import javax.json.Json;
import javax.json.JsonArrayBuilder;
import javax.json.JsonObject;
import javax.json.JsonObjectBuilder;

public class Function extends Node implements Node.IFunction {

    protected IIdentifier id;
    protected List<IPattern> params;
    protected IBlockStatement body;

    public Function(IIdentifier id, List<IPattern> params, IBlockStatement body) {
        type = FUNCTION;
        this.id = id;
        this.params = params;
        this.body = body;
    }

    @Override
    public JsonObject getEsTree() {
        // TODO Auto-generated method stub
        JsonArrayBuilder paramsJb = Json.createArrayBuilder();
        for (IPattern param : params) {
            paramsJb.add(param.getEsTree());
        }
        JsonObjectBuilder jb = Json.createObjectBuilder()
                .add(KEY_TYPE, "Function")
                .add(KEY_PARAMS, paramsJb)
                .add(KEY_BODY, body.getEsTree());
        if (id != null) {
            jb.add(KEY_ID, id.getEsTree());
        } else {
            jb.addNull(KEY_ID);
        }
        return jb.build();
    }

    @Override
    public IIdentifier getId() {
        // TODO Auto-generated method stub
        return id;
    }

    @Override
    public List<IPattern> getParams() {
        // TODO Auto-generated method stub
        return params;
    }

    @Override
    public IBlockStatement getBody() {
        // TODO Auto-generated method stub
        return body;
    }

    @Override
    public Object accept(ESTreeBaseVisitor visitor) {
        // TODO Auto-generated method stub
        return visitor.visitFunction(this);
    }

    @Override
    public void setId(IIdentifier id) {
        // TODO Auto-generated method stub
        this.id = id;
    }

    @Override
    public void setParams(List<IPattern> params) {
        // TODO Auto-generated method stub
        this.params = params;
    }

    @Override
    public void setBody(IBlockStatement body) {
        // TODO Auto-generated method stub
        this.body = body;
    }

}
