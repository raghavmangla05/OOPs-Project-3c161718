#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
#include <memory>
#include <exception>
#include <limits>
#include <cmath>

// Project: Post-Earthquake Multi-Agency Emergency Relief Logistics Hub (ERL-HUB)
// CIC-211 OOP Using C++ | Standard: C++17

namespace ReliefOps {
namespace Inventory {

class CargoOverweightException : public std::exception {
public: const char* what() const noexcept override { return "Cargo exceeds the vehicle's payload capacity."; }
};
class FuelDepletionException : public std::exception {
public: const char* what() const noexcept override { return "Vehicle fuel/range is insufficient for this route."; }
};
class DamagedCorridorException : public std::exception {
public: const char* what() const noexcept override { return "Route corridor is reported damaged or blocked."; }
};


class ReliefCargo {
    std::string itemName;
    int quantity;
    double unitWeightKg;
public:
    ReliefCargo(std::string name = "Supply", int qty = 0, double weight = 1.0)
        : itemName(std::move(name)), quantity(qty), unitWeightKg(weight) {}
    const std::string& name() const { return itemName; }
    int getQuantity() const { return quantity; }
    double unitWeight() const { return unitWeightKg; }
    double totalWeight() const { return quantity * unitWeightKg; }
    void setQuantity(int q) { if (q >= 0) quantity = q; }
    ReliefCargo& operator+=(int amount) { if (amount > 0) quantity += amount; return *this; }
    ReliefCargo& operator-=(int amount) {
        if (amount < 0 || amount > quantity) throw std::invalid_argument("Insufficient cargo quantity.");
        quantity -= amount; return *this;
    }
    bool operator==(const ReliefCargo& other) const {
        return itemName == other.itemName && quantity == other.quantity && unitWeightKg == other.unitWeightKg;
    }
    friend std::ostream& operator<<(std::ostream& os, const ReliefCargo& c) {
        return os << std::left << std::setw(22) << c.itemName << std::right << std::setw(8) << c.quantity
                  << std::setw(12) << std::fixed << std::setprecision(2) << c.unitWeightKg
                  << std::setw(14) << c.totalWeight();
    }
    friend std::istream& operator>>(std::istream& is, ReliefCargo& c) { is >> c.quantity; return is; }
};

class DistributionDepot {
    std::string depotName;
    std::map<std::string, ReliefCargo> stock;
public:
    static int totalUnitsRestocked;
    explicit DistributionDepot(std::string name = "Central Relief Depot") : depotName(std::move(name)) {
        stock.emplace("Ration Kits", ReliefCargo("Ration Kits", 120, 2.5));
        stock.emplace("Medical Kits", ReliefCargo("Medical Kits", 80, 1.8));
        stock.emplace("Water Packs", ReliefCargo("Water Packs", 150, 5.0));
        stock.emplace("Blankets", ReliefCargo("Blankets", 100, 1.2));
    }
    const std::string& name() const { return depotName; }
    std::map<std::string, ReliefCargo>& getStock() { return stock; }
    ReliefCargo& item(const std::string& key) {
        auto it = stock.find(key); if (it == stock.end()) throw std::invalid_argument("Supply not found in depot.");
        return it->second;
    }
    // Overloaded allocation routines: volume/quantity-based and emergency priority-based.
    bool allocate(const std::string& key, int qty) {
        ReliefCargo& c = item(key); if (qty < 0 || qty > c.getQuantity()) return false;
        c -= qty; return true;
    }
    bool allocate(const std::string& key, int qty, bool emergencyMedicalPriority) {
        if (emergencyMedicalPriority && key != "Medical Kits") return false;
        return allocate(key, qty);
    }
    void restock(const std::string& key, int qty) { item(key) += qty; totalUnitsRestocked += qty; }
    static int restockedUnits() { return totalUnitsRestocked; }
    friend class IncidentCommander;
};
int DistributionDepot::totalUnitsRestocked = 0;


class IncidentCommander {
public:
    // Friend access used for a critical stock override (audited in the incident log).
    static bool emergencyOverride(DistributionDepot& depot, const std::string& key, int qty) {
        auto it = depot.stock.find(key); if (it == depot.stock.end() || qty < 0) return false;
        it->second += qty; DistributionDepot::totalUnitsRestocked += qty; return true;
    }
};


class DynamicManifest {
    ReliefCargo* data;
    std::size_t count;
public:
    DynamicManifest() : data(nullptr), count(0) {}
    DynamicManifest(const ReliefCargo* items, std::size_t n) : data(n ? new ReliefCargo[n] : nullptr), count(n) {
        for (std::size_t i=0;i<n;++i) data[i]=items[i];
    }
    DynamicManifest(const DynamicManifest& other) : data(other.count ? new ReliefCargo[other.count] : nullptr), count(other.count) {
        for (std::size_t i=0;i<count;++i) data[i]=other.data[i];
    }
    DynamicManifest& operator=(const DynamicManifest& other) {
        if (this == &other) return *this;
        ReliefCargo* fresh = other.count ? new ReliefCargo[other.count] : nullptr;
        for (std::size_t i=0;i<other.count;++i) fresh[i]=other.data[i];
        delete[] data; data=fresh; count=other.count; return *this;
    }
    ~DynamicManifest() { delete[] data; }
    void add(const ReliefCargo& cargo) {
        ReliefCargo* fresh = new ReliefCargo[count+1];
        for (std::size_t i=0;i<count;++i) fresh[i]=data[i];
        fresh[count]=cargo; delete[] data; data=fresh; ++count;
    }
    std::size_t size() const { return count; }
    const ReliefCargo& operator[](std::size_t i) const { return data[i]; }
    double totalWeight() const { double w=0; for (std::size_t i=0;i<count;++i) w+=data[i].totalWeight(); return w; }
};

} // Inventory
namespace Logistics {

inline double distanceKm(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R=6371.0, pi=3.14159265358979323846;
    auto rad=[&](double x){ return x*pi/180.0; };
    double dLat=rad(lat2-lat1), dLon=rad(lon2-lon1);
    double a=std::sin(dLat/2)*std::sin(dLat/2)+std::cos(rad(lat1))*std::cos(rad(lat2))*std::sin(dLon/2)*std::sin(dLon/2);
    return 2*R*std::asin(std::sqrt(a));
}
struct GPSBeacon { double latitude=28.6139, longitude=77.2090; };
struct EmergencyLifeSupportUnit { bool operational=true; int medicalCapacity=8; };

class ReliefVehicle {
protected:
    std::string id;
    double payloadCapacityKg;
    double maxRangeKm;
    GPSBeacon gps;
    EmergencyLifeSupportUnit lifeSupport;
    static int operationalVehicles;
public:
    ReliefVehicle(std::string vehicleId, double capacity, double range)
        : id(std::move(vehicleId)), payloadCapacityKg(capacity), maxRangeKm(range) { ++operationalVehicles; }
    virtual ~ReliefVehicle() { --operationalVehicles; }
    ReliefVehicle(const ReliefVehicle&) = delete;
    ReliefVehicle& operator=(const ReliefVehicle&) = delete;
    virtual std::string type() const=0;
    virtual double routeFactor() const=0;
    virtual void evaluateRoute(double km, bool corridorDamaged) const {
        if (corridorDamaged) throw Inventory::DamagedCorridorException();
        if (km > maxRangeKm) throw Inventory::FuelDepletionException();
        std::cout << type() << " " << id << " route: " << km << " km, estimated travel " << km/routeFactor() << " hours.\n";
    }
    void dispatch(const Inventory::DynamicManifest& manifest) {
        if (manifest.totalWeight() > payloadCapacityKg) throw Inventory::CargoOverweightException();
        std::cout << "Dispatch accepted for " << type() << " [" << id << "] with " << manifest.totalWeight() << " kg cargo.\n";
    }
    const std::string& getId() const { return id; }
    static int activeVehicles() { return operationalVehicles; }
};
int ReliefVehicle::operationalVehicles=0;


class GroundConvoy : virtual public ReliefVehicle {
public:
    GroundConvoy(std::string id, double capacity=5000, double range=450) : ReliefVehicle(std::move(id),capacity,range) {}
    std::string type() const override { return "Ground Convoy"; }
    double routeFactor() const override { return 45.0; }
};
class AerialTransport : virtual public ReliefVehicle {
public:
    AerialTransport(std::string id, double capacity=1200, double range=700) : ReliefVehicle(std::move(id),capacity,range) {}
    std::string type() const override { return "Aerial Transport"; }
    double routeFactor() const override { return 180.0; }
};
class AmphibiousCraft : public ReliefVehicle {
public:
    AmphibiousCraft(std::string id, double capacity=2000, double range=250) : ReliefVehicle(std::move(id),capacity,range) {}
    std::string type() const override { return "Amphibious Craft"; }
    double routeFactor() const override { return 25.0; }
};
class HybridMedicalUnit : public GroundConvoy, public AerialTransport {
public:
    explicit HybridMedicalUnit(std::string id) : ReliefVehicle(id,1500,600), GroundConvoy(id,1500,600), AerialTransport(id,1500,600) {}
    std::string type() const override { return "Hybrid Medical Unit"; }
    double routeFactor() const override { return 65.0; }
};

struct Mission {
    std::string destination;
    int urgency; // 1 = normal, 5 = critical
    double distanceKm;
    std::string supply;
    int quantity;
};
class MissionPriority {
public: bool operator()(const Mission& a, const Mission& b) const { return a.urgency < b.urgency; }
};
template<class T> class PriorityDispatchQueue {
    std::priority_queue<T,std::vector<T>,MissionPriority> q;
public:
    void push(const T& value) { q.push(value); }
    bool empty() const { return q.empty(); }
    T pop() { if(q.empty()) throw std::runtime_error("Dispatch queue is empty."); T v=q.top(); q.pop(); return v; }
};
struct UrgencySort { bool operator()(const Mission& a, const Mission& b) const { return a.urgency > b.urgency; } };

} // Logistics
} // ReliefOps

using namespace ReliefOps::Inventory;
using namespace ReliefOps::Logistics;


// Custom tabular manipulator
std::ostream& tableHeader(std::ostream& os) {
    return os << std::left << std::setw(22) << "Supply Item" << std::right << std::setw(8) << "Qty"
              << std::setw(12) << "Unit kg" << std::setw(14) << "Total kg" << "\n" << std::string(56,'-') << "\n";
}
void saveIncident(const std::string& description, int urgency, double distance) {
    // Binary file record; seek to end for append-style persistence.
    std::fstream file("incident_log.bin", std::ios::binary|std::ios::in|std::ios::out);
    if (!file) file.open("incident_log.bin", std::ios::binary|std::ios::out);
    file.seekp(0, std::ios::end);
    std::size_t n=description.size();
    file.write(reinterpret_cast<const char*>(&n), sizeof(n));
    file.write(description.data(), static_cast<std::streamsize>(n));
    file.write(reinterpret_cast<const char*>(&urgency), sizeof(urgency));
    file.write(reinterpret_cast<const char*>(&distance), sizeof(distance));
    file.close();
}
void showIncidentLog() {
    std::ifstream file("incident_log.bin", std::ios::binary);
    if (!file) { std::cout << "No incident log found yet.\n"; return; }
    std::cout << "\n--- Saved Incident Records ---\n";
    while (file) {
        std::size_t n=0; file.read(reinterpret_cast<char*>(&n),sizeof(n));
        if (!file || n>10000) break;
        std::string desc(n,'\0'); file.read(&desc[0],static_cast<std::streamsize>(n));
        int urgency=0; double distance=0;
        file.read(reinterpret_cast<char*>(&urgency),sizeof(urgency));
        file.read(reinterpret_cast<char*>(&distance),sizeof(distance));
        if (!file) break;
        std::cout << desc << " | urgency " << urgency << " | " << distance << " km\n";
    }
}

void printInventory(DistributionDepot& depot) {
    std::cout << "\nDepot: " << depot.name() << "\n"; tableHeader(std::cout);
    for (const auto& p : depot.getStock()) std::cout << p.second << "\n";
}
void restockMenu(DistributionDepot& depot) {
    std::cout << "Supply names: Ration Kits, Medical Kits, Water Packs, Blankets\nEnter exact supply name: ";
    std::string name; std::getline(std::cin,name);
    std::cout << "Quantity to restock: "; int qty; std::cin >> qty; std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    try { depot.restock(name,qty); std::cout << "Restock complete.\n"; }
    catch(const std::exception& e) { std::cout << "Error: " << e.what() << "\n"; }
}
void dispatchMission(DistributionDepot& depot, std::vector<std::unique_ptr<ReliefVehicle>>& fleet) {
    std::cout << "\nDestination: "; std::string dest; std::getline(std::cin,dest);
    std::cout << "Urgency (1-5): "; int urgency; std::cin >> urgency;
    std::cout << "Distance (km): "; double km; std::cin >> km;
    std::cout << "Supply (Ration Kits / Medical Kits / Water Packs / Blankets): "; std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    std::string supply; std::getline(std::cin,supply);
    std::cout << "Quantity: "; int qty; std::cin >> qty;
    std::cout << "Is route damaged? (1=yes, 0=no): "; int damaged; std::cin >> damaged; std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    try {
        if (urgency<1 || urgency>5 || qty<=0 || km<0) throw std::invalid_argument("Invalid mission input.");
        ReliefCargo cargo(supply,qty, depot.getStock().at(supply).unitWeight());
        DynamicManifest manifest; manifest.add(cargo); // dynamic buffer and deep-copy support
        // Sort/priority queue functor and template queue demonstrate priority dispatch.
        Mission m{dest,urgency,km,supply,qty};
        PriorityDispatchQueue<Mission> queue; queue.push(m); Mission selected=queue.pop();
        std::sort(fleet.begin(),fleet.end(),[](const auto& a,const auto& b){ return a->routeFactor()>b->routeFactor(); });
        ReliefVehicle* chosen=nullptr;
        for (auto& v : fleet) { try { v->evaluateRoute(selected.distanceKm, damaged!=0); chosen=v.get(); break; } catch(const std::exception&) {} }
        if (!chosen) throw std::runtime_error("No available vehicle can safely cover this route.");
        chosen->dispatch(manifest);
        if (!depot.allocate(supply,qty,urgency>=4 && supply=="Medical Kits")) throw std::runtime_error("Insufficient supply or medical priority mismatch.");
        saveIncident("Dispatch to " + dest + " via " + chosen->type() + " (" + chosen->getId() + ")", urgency, km);
        std::cout << "Mission recorded to incident_log.bin.\n";
    } catch(const std::exception& e) {
        std::cout << "Dispatch failed: " << e.what() << "\n";
        saveIncident("FAILED dispatch to " + dest + ": " + e.what(), urgency, km);
    }
}

int main() {
    DistributionDepot depot("MAIT Relief Coordination Depot");
    std::vector<std::unique_ptr<ReliefVehicle>> fleet;
    fleet.emplace_back(new GroundConvoy("GC-01",5000,450));
    fleet.emplace_back(new AerialTransport("AT-01",1200,700));
    fleet.emplace_back(new AmphibiousCraft("AC-01",2000,250));
    fleet.emplace_back(new HybridMedicalUnit("HM-01"));
    bool running=true;
    while(running) {
        std::cout << "\n========== ERL-HUB: Emergency Relief Logistics ==========" <<
                  "\n1. View depot inventory\n2. Restock supplies\n3. Create and dispatch relief mission\n4. View saved incident log\n5. Network statistics\n0. Exit\nChoice: ";
        int choice; if(!(std::cin>>choice)) break; std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        switch(choice) {
            case 1: printInventory(depot); break;
            case 2: restockMenu(depot); break;
            case 3: dispatchMission(depot,fleet); break;
            case 4: showIncidentLog(); break;
            case 5: std::cout << "Operational vehicles: " << ReliefVehicle::activeVehicles() << " | Units restocked: " << DistributionDepot::restockedUnits() << "\n"; break;
            case 0: running=false; std::cout << "Exiting ERL-HUB. Stay safe.\n"; break;
            default: std::cout << "Invalid choice. Try again.\n";
        }
    }
    return 0;
}
