/////////////////////////////////////////////////////
// All map assets for this toy came from: https://github.com/themanaworld/tmwa-client-data
/////////////////////////////////////////////////////

function TmxMapToy::create( %this )
{
    // Set the sandbox drag mode availability.
    Sandbox.allowManipulation( pan );
    
    // Set the manipulation mode.
    Sandbox.useManipulation( pan );
    
    // Reset the toy.
    TmxMapToy.reset();
}


//-----------------------------------------------------------------------------

function TmxMapToy::destroy( %this )
{
}

//-----------------------------------------------------------------------------

function TmxMapToy::reset( %this )
{
    // Clear the scene.  
    SandboxScene.clear();
    
    %mapSprite = new TmxMapSprite()
    {
       Map = "ToyAssets:testtown_map";
    };    
    SandboxScene.add( %mapSprite );
}

//-----------------------------------------------------------------------------
