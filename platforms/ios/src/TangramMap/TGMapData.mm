//
//  TGMapData.mm
//  TangramMap
//
//  Created by Karim Naaji on 2/24/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGMapViewController+Internal.h"
#import <memory>
#import <vector>

@interface TGMapData () {
    std::shared_ptr<Tangram::ClientGeoJsonSource> dataSource;
}

@property (copy, nonatomic) NSString* name;
@property (weak, nonatomic) TGMapViewController* mapView;

@end

@implementation TGMapData

- (instancetype)initWithMapView:(TGMapViewController *)mapView name:(NSString *)name
{
    self = [super init];

    if (self) {
        self.name = name;
        self.mapView = mapView;

        // Create client data source with Tangram
        dataSource = [self.mapView createDataSource:name];
    }

    return self;
}

- (void)addPoint:(TGGeoPoint)point withProperties:(FeatureProperties *)properties
{
    // TODO
}

- (void)addPolygon:(TGGeoPolygon *)polygon withProperties:(FeatureProperties *)properties
{
    // TODO
}

- (void)addPolyline:(TGGeoPolyline *)polyline withProperties:(FeatureProperties *)properties
{
    // TODO
}

- (void)addGeoJson:(NSString *)data
{
    std::string sourceData = std::string([data UTF8String]);
    dataSource->addData(sourceData);
}

- (void)clear
{
    // TODO
}

@end
