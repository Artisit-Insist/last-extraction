import unreal, os
from pathlib import Path
root=str(Path(__file__).resolve().parent.parent)
a=unreal.AssetToolsHelpers.get_asset_tools()
for name in ['JungleKeyArt','EquipmentAtlas']:
 t=unreal.AssetImportTask(); t.filename=root+'/ArtSource/'+name+'.png'; t.destination_path='/Game/Art'; t.automated=True; t.save=True; a.import_asset_tasks([t])
# A simple physically based surface, with procedural large and small scale color variation.
m=a.create_asset('M_Surface','/Game/Materials',unreal.Material,unreal.MaterialFactoryNew())
e=unreal.MaterialEditingLibrary
color=e.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-600,0); color.set_editor_property('parameter_name','Tint'); color.set_editor_property('default_value',unreal.LinearColor(.18,.25,.1,1))
noise=e.create_material_expression(m,unreal.MaterialExpressionNoise,-600,220); noise.set_editor_property('scale',.07); noise.set_editor_property('quality',1)
mul=e.create_material_expression(m,unreal.MaterialExpressionMultiply,-200,0); mul.set_editor_property('const_b',1)
add=e.create_material_expression(m,unreal.MaterialExpressionAdd,-400,200); add.set_editor_property('const_b',.45)
e.connect_material_expressions(noise,'',add,'A'); e.connect_material_expressions(color,'',mul,'A'); e.connect_material_expressions(add,'',mul,'B'); e.connect_material_property(mul,'',unreal.MaterialProperty.MP_BASE_COLOR)
r=e.create_material_expression(m,unreal.MaterialExpressionScalarParameter,-200,360); r.set_editor_property('parameter_name','Roughness'); r.set_editor_property('default_value',.72); e.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
metal=e.create_material_expression(m,unreal.MaterialExpressionScalarParameter,-200,460); metal.set_editor_property('parameter_name','Metallic'); metal.set_editor_property('default_value',0); e.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
e.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m)
em=a.create_asset('M_Glow','/Game/Materials',unreal.Material,unreal.MaterialFactoryNew()); c=e.create_material_expression(em,unreal.MaterialExpressionVectorParameter); c.set_editor_property('parameter_name','Tint'); c.set_editor_property('default_value',unreal.LinearColor(1,.4,.05,1)); e.connect_material_property(c,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR); e.recompile_material(em); unreal.EditorAssetLibrary.save_loaded_asset(em)
sub=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
sub.new_level('/Game/Maps/Jungle'); sub.save_current_level()
unreal.log('EXTRACTION_ASSETS_READY')
