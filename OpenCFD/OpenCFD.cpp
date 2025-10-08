// OpenCFD.cpp : Defines the entry point for the application.
// 2D Lattice Boltzmann (D2Q9) CPU implementation with Raylib visualization

#include "OpenCFD.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>

using namespace std;

// Domain size
static const int NX = 400;
static const int NY = 200;

// LBM D2Q9
static const int Q = 9;
static const int ex[Q] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
static const int ey[Q] = {0, 0, 1, 0, -1, 1, 1, -1, -1};
static const float w[Q] = {4.f/9.f, 1.f/9.f, 1.f/9.f, 1.f/9.f, 1.f/9.f, 1.f/36.f, 1.f/36.f, 1.f/36.f, 1.f/36.f};
static const int opp[Q] = {0, 3, 4, 1, 2, 7, 8, 5, 6};

inline int idx(int x, int y) { return y * NX + x; }

int main()
{
    // Window and visualization settings
    const int winW = NX * 2; // scale up
    const int winH = NY * 2;
    InitWindow(winW, winH, "OpenCFD - LBM D2Q9 (CPU)");
    SetTargetFPS(60);

    // Simulation parameters
    const float u_in = 0.08f; // inlet velocity
    const float Re = 100.0f;  // Reynolds number (for viscosity estimate)
    const float radius = (float)(NY / 9); // cylinder radius
    const int cx = NX/4; // cylinder center x
    const int cy = NY/2; // cylinder center y

    // tau (relaxation time) from viscosity: nu = (tau-0.5)/3. Choose tau to match Re roughly
    const float characteristic_length = 2.0f * radius;
    // choose viscosity nu = u_in * L / Re
    const float nu = u_in * characteristic_length / Re;
    const float tau = 3.f * nu + 0.5f;

    // Allocate fields
    const int N = NX * NY;
    vector<float> f(Q * N);
    vector<float> f2(Q * N);
    vector<float> rho(N);
    vector<float> ux(N);
    vector<float> uy(N);
    vector<unsigned char> pixels(4 * N); // RGBA8 for texture update

    // obstacle mask
    vector<unsigned char> obstacle(N);
    for (int y = 0; y < NY; ++y)
    {
        for (int x = 0; x < NX; ++x)
        {
            float dx = (float)(x - cx);
            float dy = (float)(y - cy);
            if (dx*dx + dy*dy <= radius*radius) obstacle[idx(x,y)] = 1;
            else obstacle[idx(x,y)] = 0;
        }
    }

    // Initialize to equilibrium with small uniform inlet velocity
    for (int y = 0; y < NY; ++y)
    {
        for (int x = 0; x < NX; ++x)
        {
            int id = idx(x,y);
            float u0 = u_in; // initial background velocity
            float ux0 = u0;
            float uy0 = 0.f;
            float rho0 = 1.f;
            rho[id] = rho0;
            ux[id] = ux0;
            uy[id] = uy0;
            for (int k = 0; k < Q; ++k)
            {
                float eu = ex[k]*ux0 + ey[k]*uy0;
                float usq = ux0*ux0 + uy0*uy0;
                f[k*N + id] = w[k] * rho0 * (1.f + 3.f*eu + 4.5f*eu*eu - 1.5f*usq);
            }
        }
    }

    // Create image and texture for visualization
    Image img = GenImageColor(NX, NY, BLACK);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    // Main loop
    int stepsPerFrame = 4; // do several LBM steps per frame for faster dynamics
    while (!WindowShouldClose())
    {
        // perform several LBM steps
        for (int step = 0; step < stepsPerFrame; ++step)
        {
            // Compute macroscopic variables (rho, u) from f
            for (int y = 0; y < NY; ++y)
            for (int x = 0; x < NX; ++x)
            {
                int id = idx(x,y);
                if (obstacle[id]) { rho[id] = 1.f; ux[id] = 0.f; uy[id] = 0.f; continue; }
                float local_rho = 0.f;
                float local_ux = 0.f;
                float local_uy = 0.f;
                for (int k = 0; k < Q; ++k)
                {
                    float fk = f[k*N + id];
                    local_rho += fk;
                    local_ux += fk * ex[k];
                    local_uy += fk * ey[k];
                }
                rho[id] = local_rho;
                ux[id] = local_ux / local_rho;
                uy[id] = local_uy / local_rho;
            }

            // Collision (BGK)
            for (int y = 0; y < NY; ++y)
            for (int x = 0; x < NX; ++x)
            {
                int id = idx(x,y);
                if (obstacle[id]) continue;
                float usq = ux[id]*ux[id] + uy[id]*uy[id];
                for (int k = 0; k < Q; ++k)
                {
                    float eu = ex[k]*ux[id] + ey[k]*uy[id];
                    float feq = w[k] * rho[id] * (1.f + 3.f*eu + 4.5f*eu*eu - 1.5f*usq);
                    // Relaxation
                    f[k*N + id] += -(f[k*N + id] - feq) / tau;
                }
            }

            // Streaming step: f2(x+e) = f(x)
            // Initialize f2 to zero for clarity
            std::fill(f2.begin(), f2.end(), 0.f);
            for (int y = 0; y < NY; ++y)
            for (int x = 0; x < NX; ++x)
            {
                int id = idx(x,y);
                for (int k = 0; k < Q; ++k)
                {
                    int xt = x + ex[k];
                    int yt = y + ey[k];
                    // simple boundary conditions: periodic top/bottom, bounce on left/right handled separately
                    if (yt < 0) yt = NY - 1;
                    if (yt >= NY) yt = 0;
                    if (xt < 0) xt = 0; // left boundary handled as inlet
                    if (xt >= NX) xt = NX - 1; // right boundary handled as outlet
                    int idt = idx(xt, yt);
                    f2[k*N + idt] = f[k*N + id];
                }
            }

            // Bounce-back for obstacles
            for (int y = 0; y < NY; ++y)
            for (int x = 0; x < NX; ++x)
            {
                int id = idx(x,y);
                if (!obstacle[id]) continue;
                // for obstacle cells, swap incoming populations
                // we set f2[k,id] = f[opp(k),id]
                for (int k = 0; k < Q; ++k)
                {
                    f2[k*N + id] = f[opp[k]*N + id];
                }
            }

            // Inlet: left column - set to equilibrium with prescribed u_in
            for (int y = 0; y < NY; ++y)
            {
                int x = 0;
                int id = idx(x,y);
                float uxb = u_in;
                float uyb = 0.f;
                float rhob = 0.f;
                // compute rho from unknown populations (using simple approach: set rho to 1)
                rhob = 1.f;
                for (int k = 0; k < Q; ++k)
                {
                    float eu = ex[k]*uxb + ey[k]*uyb;
                    float feq = w[k] * rhob * (1.f + 3.f*eu + 4.5f*eu*eu - 1.5f*(uxb*uxb+uyb*uyb));
                    f2[k*N + id] = feq;
                }
            }

            // Outlet: right column - simple zero-gradient (copy from left neighbor)
            for (int y = 0; y < NY; ++y)
            {
                int x = NX-1;
                int id = idx(x,y);
                int idm = idx(NX-2, y);
                for (int k = 0; k < Q; ++k) f2[k*N + id] = f2[k*N + idm];
            }

            // swap
            f.swap(f2);
        }

        // After steps, compute speed and fill pixels
        float maxSpeed = 0.f;
        for (int i = 0; i < N; ++i)
        {
            float u = ux[i];
            float v = uy[i];
            float s = sqrtf(u*u + v*v);
            if (s > maxSpeed) maxSpeed = s;
        }
        // avoid division by zero
        float scale = 1.f / (maxSpeed + 1e-6f);

        for (int y = 0; y < NY; ++y)
        for (int x = 0; x < NX; ++x)
        {
            int id = idx(x,y);
            unsigned char r, g, b, a;
            if (obstacle[id]) {
                r = 80; g = 80; b = 80; a = 255;
            } else {
                float s = sqrtf(ux[id]*ux[id] + uy[id]*uy[id]) * scale;
                // simple colormap: blue (low) -> yellow (mid) -> red (high)
                float t = fminf(1.f, s*2.f);
                if (t < 0.5f) {
                    float tt = t*2.f;
                    r = (unsigned char)(tt * 255);
                    g = (unsigned char)(tt * 128);
                    b = (unsigned char)(255 * (1.f - tt*0.5f));
                } else {
                    float tt = (t-0.5f)*2.f;
                    r = (unsigned char)(255);
                    g = (unsigned char)(255*(1.f - 0.8f*tt));
                    b = (unsigned char)(64*(1.f - tt));
                }
                a = 255;
            }
            int p = 4 * id;
            pixels[p + 0] = r;
            pixels[p + 1] = g;
            pixels[p + 2] = b;
            pixels[p + 3] = a;
        }

        // Update texture from pixels
        UpdateTexture(tex, pixels.data());

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);
        // draw the texture scaled to window
        Vector2 pos = {0.f, 0.f};
        DrawTextureEx(tex, pos, 0.f, (float)winW / NX, WHITE);
        DrawFPS(10,10);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();

    return 0;
}
