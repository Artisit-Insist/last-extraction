import unreal
from pathlib import Path
root=str(Path(__file__).resolve().parent.parent)
a=unreal.AssetToolsHelpers.get_asset_tools();e=unreal.MaterialEditingLibrary
if not unreal.EditorAssetLibrary.does_asset_exist('/Game/Art/JungleGround'):
 t=unreal.AssetImportTask();t.filename=root+'/ArtSource/JungleGround.png';t.destination_path='/Game/Art';t.automated=True;t.save=True;a.import_asset_tasks([t])
m=a.create_asset('M_Ground','/Game/Materials',unreal.Material,unreal.MaterialFactoryNew())
tex=e.create_material_expression(m,unreal.MaterialExpressionTextureSample,-300,0);tex.set_editor_property('texture',unreal.load_asset('/Game/Art/JungleGround'))
pos=e.create_material_expression(m,unreal.MaterialExpressionWorldPosition,-900,0)
mask=e.create_material_expression(m,unreal.MaterialExpressionComponentMask,-730,0);mask.set_editor_property('r',True);mask.set_editor_property('g',True);mask.set_editor_property('b',False);mask.set_editor_property('a',False)
scale=e.create_material_expression(m,unreal.MaterialExpressionMultiply,-530,0);scale.set_editor_property('const_b',.002)
e.connect_material_expressions(pos,'',mask,'');e.connect_material_expressions(mask,'',scale,'A');e.connect_material_expressions(scale,'',tex,'UVs');e.connect_material_property(tex,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
r=e.create_material_expression(m,unreal.MaterialExpressionConstant);r.set_editor_property('r',.84);e.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
e.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('GROUND_READY')
