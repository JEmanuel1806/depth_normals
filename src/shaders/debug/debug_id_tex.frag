#version 440 core
uniform isampler2D idTex;
out vec4 FragColor;

void main(){
    ivec2 p = ivec2(gl_FragCoord.xy);
    int id  = texelFetch(idTex, p, 0).r;

    if (id < 0) 
    { 
        FragColor = vec4(0,0,0,0); 
        return; 
    } 
       else if (id == 1) 
    {
        FragColor = vec4(1.0, 0.0, 0.0, 0.0);
        return;
    }
       else if (id == 2) 
    {
        FragColor = vec4(0.0, 1.0, 0.0, 0.0);
        return;
    }
       else if (id == 3) 
    {
        FragColor = vec4(0.0, 0.0, 1.0, 0.0);
        return;
    }
       else if (id == 4) 
    {
        FragColor = vec4(1.0, 1.0, 0.0, 0.0);
        return;
    }
       else if (id == 5) 
    {
        FragColor = vec4(0.0, 1.0, 1.0, 0.0);
        return;
    }
       else if (id == 6) 
    {
        FragColor = vec4(1.0, 1.0, 1.0, 0.0);
        return;
    }
       else if (id == 7) 
    {
        FragColor = vec4(1.0, 0.0, 1.0, 0.0);
        return;
    }
           else if (id == 8) 
    {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
           else if (id == 0) 
    {
        FragColor = vec4(1.0, 0.5, 1.0, 0.0);
        return;
    }
    else if (id == 1041261120.000000) 
    {
        FragColor = vec4(0.5,0.5,0.5, 0.0);
        return;
    }
    FragColor = vec4(0.5,0.5,0.5, 1.0);
}