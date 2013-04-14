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
    
    //add a simple dynamic sprite to collide with the tiles
    %db = new Sprite(TestAnimation)
    {
        Animation = "ToyAssets:TD_Knight_MoveWest";
        position = "2 0";
        size = "1.5 1.5";
        SceneLayer = "14";
        DefaultDensity = 0.5;
        DefaultFriction = 0.1;
        DefaultRestitution = 0.9;
    };
    
   %db.createCircleCollisionShape(0.2);  
   %db.setFixedAngle(true);
   %db.setLinearVelocity("5 3");
    
   SandboxScene.add( %db );
     
}

//-----------------------------------------------------------------------------
