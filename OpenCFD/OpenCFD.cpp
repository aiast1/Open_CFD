// OpenCFD.cpp : Fast-Moving Air 2D Lattice Boltzmann CFD Simulation
// High-speed, low-viscosity air simulation with dynamic motion

#include "OpenCFD.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

// Domain size
const int NX = 400;
const int NY = 200;
const int Q = 9;

// D2Q9 lattice vectors and weights
const int ex[Q] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
const int ey[Q] = {0, 0, 1, 0, -1, 1, 1, -1, -1};
const float w[Q] = {4.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f};
const int opp[Q] = {0, 3, 4, 1, 2, 7, 8, 5, 6};

inline int idx(int x, int y) { return y * NX + x; }

class FastAirLBM {
private:
    vector<vector<float>> f; // Distribution functions [Q][N]
    vector<float> rho, ux, uy;
    vector<bool> obstacle;
    vector<Color> pixels;
    
    float tau;
    float u_in;
    int time_step;
    
    Texture2D texture;

public:
    FastAirLBM() {
        int N = NX * NY;
        
        // Initialize distribution functions
        f.resize(Q);
        for (int k = 0; k < Q; k++) {
            f[k].resize(N);
        }
        
        rho.resize(N);
        ux.resize(N);
        uy.resize(N);
        obstacle.resize(N);
        pixels.resize(N);
        
        // FAST AIR PARAMETERS - Much higher velocity, lower viscosity
        u_in = 0.25f;  // Very high velocity for fast motion
        float Re = 1000.0f; // High Reynolds number = low viscosity = fast moving air
        float radius = NY / 9.0f;
        float nu = u_in * (2.0f * radius) / Re; // Very low viscosity
        tau = 3.0f * nu + 0.5f;
        
        // Force tau to be as low as possible for fast air
        if (tau < 0.51f) tau = 0.51f; // Minimum for stability
        if (tau > 0.8f) tau = 0.8f;   // Maximum for fast motion
        
        time_step = 0;
        
        cout << "Fast Air LBM CFD Initialized" << endl;
        cout << "Domain: " << NX << " x " << NY << endl;
        cout << "HIGH-SPEED INLET VELOCITY: " << u_in << endl;
        cout << "HIGH Reynolds (low viscosity): " << Re << endl;
        cout << "LOW Tau (fast air): " << tau << endl;
        cout << "Air moves VERY FREELY and FAST!" << endl;
    }
    
    void Initialize() {
        // Create circular obstacle
        int cx = NX / 4;
        int cy = NY / 2;
        float R = NY / 9.0f;
        
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int id = idx(x, y);
                
                float dx = x - cx;
                float dy = y - cy;
                obstacle[id] = (dx*dx + dy*dy <= R*R);
            }
        }
        
        // Initialize fast-moving air flow field
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int id = idx(x, y);
                
                rho[id] = 1.0f;
                
                if (obstacle[id]) {
                    ux[id] = 0.0f;
                    uy[id] = 0.0f;
                } else {
                    // Less parabolic profile = more uniform fast flow
                    float y_center = (float)y - NY/2.0f;
                    float profile = 1.0f - 2.0f * (y_center/(NY/2.0f)) * (y_center/(NY/2.0f));
                    profile = max(0.2f, profile); // Minimum 20% speed everywhere
                    
                    ux[id] = u_in * profile; // Very fast air
                    uy[id] = 0.0f;
                    
                    // Strong perturbation to create dynamic instabilities
                    if (x > cx + R + 2 && x < cx + R + 20) {
                        uy[id] = 0.1f * u_in * sin(6.28f * y / (NY/4));
                    }
                }
                
                // Initialize equilibrium distributions
                ComputeEquilibrium(id);
            }
        }
        
        // Create texture
        Image img = GenImageColor(NX, NY, BLACK);
        texture = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    
    void ComputeEquilibrium(int id) {
        float density = rho[id];
        float u = ux[id];
        float v = uy[id];
        float usq = u*u + v*v;
        
        for (int k = 0; k < Q; k++) {
            float eu = ex[k]*u + ey[k]*v;
            f[k][id] = w[k] * density * (1.0f + 3.0f*eu + 4.5f*eu*eu - 1.5f*usq);
        }
    }
    
    void ComputeMacroscopic() {
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int id = idx(x, y);
                
                if (obstacle[id]) {
                    rho[id] = 1.0f;
                    ux[id] = 0.0f;
                    uy[id] = 0.0f;
                    continue;
                }
                
                float density = 0.0f;
                float vel_x = 0.0f;
                float vel_y = 0.0f;
                
                for (int k = 0; k < Q; k++) {
                    density += f[k][id];
                    vel_x += ex[k] * f[k][id];
                    vel_y += ey[k] * f[k][id];
                }
                
                // Ensure density is positive
                density = max(density, 1e-10f);
                
                rho[id] = density;
                ux[id] = vel_x / density;
                uy[id] = vel_y / density;
            }
        }
    }
    
    void Collision() {
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int id = idx(x, y);
                
                if (obstacle[id]) continue;
                
                float usq = ux[id]*ux[id] + uy[id]*uy[id];
                
                for (int k = 0; k < Q; k++) {
                    float eu = ex[k]*ux[id] + ey[k]*uy[id];
                    float feq = w[k] * rho[id] * (1.0f + 3.0f*eu + 4.5f*eu*eu - 1.5f*usq);
                    f[k][id] = f[k][id] - (f[k][id] - feq) / tau; // Low tau = fast relaxation = low viscosity
                }
            }
        }
    }
    
    void Streaming() {
        // Create temporary array for streaming
        vector<vector<float>> f_temp(Q);
        for (int k = 0; k < Q; k++) {
            f_temp[k].resize(NX * NY);
        }
        
        // Stream particles
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int id = idx(x, y);
                
                for (int k = 0; k < Q; k++) {
                    int x_new = x + ex[k];
                    int y_new = y + ey[k];
                    
                    // Periodic boundaries top/bottom
                    if (y_new < 0) y_new = NY - 1;
                    if (y_new >= NY) y_new = 0;
                    
                    // Handle left/right boundaries
                    if (x_new >= 0 && x_new < NX) {
                        int id_new = idx(x_new, y_new);
                        f_temp[k][id_new] = f[k][id];
                    }
                }
            }
        }
        
        // Handle obstacles with bounce-back
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int id = idx(x, y);
                
                if (obstacle[id]) {
                    for (int k = 0; k < Q; k++) {
                        f_temp[k][id] = f[opp[k]][id];
                    }
                }
            }
        }
        
        // Copy back
        f = f_temp;
    }
    
    void BoundaryConditions() {
        // High-speed inlet boundary (left side)
        for (int y = 0; y < NY; y++) {
            int id = idx(0, y);
            
            // Less parabolic = more uniform high-speed flow
            float y_center = (float)y - NY/2.0f;
            float profile = 1.0f - 2.0f * (y_center/(NY/2.0f)) * (y_center/(NY/2.0f));
            profile = max(0.3f, profile); // High minimum speed
            
            rho[id] = 1.0f;
            ux[id] = u_in * profile; // Very fast inlet
            uy[id] = 0.0f;
            
            ComputeEquilibrium(id);
        }
        
        // Outlet boundary (right side) - zero gradient
        for (int y = 0; y < NY; y++) {
            int id_out = idx(NX-1, y);
            int id_in = idx(NX-2, y);
            
            rho[id_out] = rho[id_in];
            ux[id_out] = ux[id_in];
            uy[id_out] = uy[id_in];
            
            for (int k = 0; k < Q; k++) {
                f[k][id_out] = f[k][id_in];
            }
        }
    }
    
    void Update() {
        // Multiple time steps for even faster dynamics
        for (int step = 0; step < 2; step++) {
            ComputeMacroscopic();
            Collision();
            Streaming();
            BoundaryConditions();
        }
        time_step += 2;
    }
    
    void Render() {
        // Find max speed for color scaling
        float max_speed = 0.0f;
        for (int i = 0; i < NX*NY; i++) {
            if (!obstacle[i]) {
                float speed = sqrt(ux[i]*ux[i] + uy[i]*uy[i]);
                max_speed = max(max_speed, speed);
            }
        }
        
        // Generate colors with enhanced contrast for fast motion
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int id = idx(x, y);
                
                if (obstacle[id]) {
                    pixels[id] = {80, 80, 80, 255}; // Dark gray obstacle
                } else {
                    float speed = sqrt(ux[id]*ux[id] + uy[id]*uy[id]);
                    float norm_speed = speed / (max_speed + 1e-10f);
                    
                    // Enhanced high-contrast color mapping for fast air
                    unsigned char r, g, b;
                    
                    if (norm_speed < 0.1f) {
                        // Very dark blue for slow/stagnant areas
                        r = 0;
                        g = 0;
                        b = (unsigned char)(50 + norm_speed * 500);
                    } else if (norm_speed < 0.3f) {
                        // Blue to cyan transition
                        float t = (norm_speed - 0.1f) * 5.0f;
                        r = 0;
                        g = (unsigned char)(t * 200);
                        b = 255;
                    } else if (norm_speed < 0.6f) {
                        // Cyan to green to yellow
                        float t = (norm_speed - 0.3f) * 3.33f;
                        r = (unsigned char)(t * 255);
                        g = 255;
                        b = (unsigned char)(255 - t * 255);
                    } else {
                        // Yellow to bright red for very fast areas
                        float t = (norm_speed - 0.6f) * 2.5f;
                        r = 255;
                        g = (unsigned char)(255 - t * 200);
                        b = 0;
                    }
                    
                    pixels[id] = {r, g, b, 255};
                }
            }
        }
        
        UpdateTexture(texture, (unsigned char*)pixels.data());
    }
    
    Texture2D GetTexture() { return texture; }
    void Cleanup() { UnloadTexture(texture); }
    float GetMaxSpeed() { 
        float max_speed = 0.0f;
        for (int i = 0; i < NX*NY; i++) {
            if (!obstacle[i]) {
                float speed = sqrt(ux[i]*ux[i] + uy[i]*uy[i]);
                max_speed = max(max_speed, speed);
            }
        }
        return max_speed;
    }
    float GetInletSpeed() { return u_in; }
    float GetReynolds() { 
        float radius = NY / 9.0f;
        return u_in * (2.0f * radius) / ((tau - 0.5f) / 3.0f); 
    }
};

int main() {
    InitWindow(NX*2, NY*2, "Fast Air LBM CFD - High Speed Low Viscosity");
    SetTargetFPS(60);
    
    FastAirLBM sim;
    sim.Initialize();
    
    cout << "FAST-MOVING AIR CFD running!" << endl;
    cout << "Air moves very freely with high speed and low viscosity!" << endl;
    
    while (!WindowShouldClose()) {
        sim.Update();
        sim.Render();
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Draw simulation
        DrawTextureEx(sim.GetTexture(), {0, 0}, 0.0f, 2.0f, WHITE);
        
        // Enhanced info display
        DrawFPS(10, 10);
        DrawText("FAST AIR LBM CFD", 10, 30, 20, WHITE);
        DrawText("High Speed - Low Viscosity", 10, 50, 16, GREEN);
        DrawText(TextFormat("Max Speed: %.3f", sim.GetMaxSpeed()), 10, 70, 16, YELLOW);
        DrawText(TextFormat("Inlet: %.3f", sim.GetInletSpeed()), 10, 90, 16, YELLOW);
        DrawText(TextFormat("Reynolds: %.0f", sim.GetReynolds()), 10, 110, 16, CYAN);
        DrawText("Dark Blue=Slow, Red=Very Fast", 10, 130, 14, WHITE);
        
        EndDrawing();
    }
    
    sim.Cleanup();
    CloseWindow();
    
    return 0;
}
