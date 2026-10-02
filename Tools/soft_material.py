import unreal
A=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.MaterialEditingLibrary
m=A.create_asset('M_SurfaceSoft','/Game/Materials',unreal.Material,unreal.MaterialFactoryNew())
c=E.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-300,0);c.set_editor_property('parameter_name','Tint');c.set_editor_property('default_value',unreal.LinearColor(.2,.2,.2,1));E.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
for name,prop,value,y in [('Roughness',unreal.MaterialProperty.MP_ROUGHNESS,.7,150),('Metallic',unreal.MaterialProperty.MP_METALLIC,0,300)]:
 p=E.create_material_expression(m,unreal.MaterialExpressionScalarParameter,-300,y);p.set_editor_property('parameter_name',name);p.set_editor_property('default_value',value);E.connect_material_property(p,'',prop)
E.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
# Matte, two-sided leaves to avoid black back faces.
m.set_editor_property('two_sided',True);E.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('SOFT_MATERIAL_READY')
